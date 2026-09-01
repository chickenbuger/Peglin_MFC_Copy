#include "pch.h"
#include "AudioPlayer.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>
#include <xaudio2.h>

#pragma comment(lib, "xaudio2.lib")

namespace
{
	constexpr float MUSIC_FADE_SECONDS = 0.28f;
	constexpr std::size_t MAX_EFFECT_VOICES = 8;

	struct ParsedWave
	{
		WAVEFORMATEX format{};
		std::size_t dataOffset = 0;
		UINT32 dataSize = 0;
	};

	std::uint16_t ReadUInt16(const std::vector<std::uint8_t>& bytes, std::size_t offset)
	{
		return static_cast<std::uint16_t>(bytes[offset])
			| static_cast<std::uint16_t>(bytes[offset + 1]) << 8U;
	}

	std::uint32_t ReadUInt32(const std::vector<std::uint8_t>& bytes, std::size_t offset)
	{
		return static_cast<std::uint32_t>(bytes[offset])
			| static_cast<std::uint32_t>(bytes[offset + 1]) << 8U
			| static_cast<std::uint32_t>(bytes[offset + 2]) << 16U
			| static_cast<std::uint32_t>(bytes[offset + 3]) << 24U;
	}

	bool ParseWave(const std::vector<std::uint8_t>& bytes, ParsedWave& parsed) noexcept
	{
		if (bytes.size() < 44U
			|| std::string_view(reinterpret_cast<const char*>(bytes.data()), 4) != "RIFF"
			|| std::string_view(reinterpret_cast<const char*>(bytes.data() + 8), 4) != "WAVE")
		{
			return false;
		}
		bool formatSeen = false;
		bool dataSeen = false;
		std::size_t offset = 12U;
		while (offset + 8U <= bytes.size())
		{
			const std::string_view id(reinterpret_cast<const char*>(bytes.data() + offset), 4);
			const std::size_t chunkSize = ReadUInt32(bytes, offset + 4U);
			const std::size_t chunkData = offset + 8U;
			if (chunkSize > bytes.size() - chunkData)
			{
				return false;
			}
			if (id == "fmt " && chunkSize >= 16U)
			{
				parsed.format.wFormatTag = ReadUInt16(bytes, chunkData);
				parsed.format.nChannels = ReadUInt16(bytes, chunkData + 2U);
				parsed.format.nSamplesPerSec = ReadUInt32(bytes, chunkData + 4U);
				parsed.format.nAvgBytesPerSec = ReadUInt32(bytes, chunkData + 8U);
				parsed.format.nBlockAlign = ReadUInt16(bytes, chunkData + 12U);
				parsed.format.wBitsPerSample = ReadUInt16(bytes, chunkData + 14U);
				parsed.format.cbSize = 0;
				formatSeen = parsed.format.wFormatTag == WAVE_FORMAT_PCM
					&& (parsed.format.nChannels == 1U || parsed.format.nChannels == 2U)
					&& parsed.format.wBitsPerSample == 16U
					&& parsed.format.nSamplesPerSec > 0U
					&& parsed.format.nBlockAlign > 0U;
			}
			else if (id == "data" && chunkSize > 0U && chunkSize <= UINT32_MAX)
			{
				parsed.dataOffset = chunkData;
				parsed.dataSize = static_cast<UINT32>(chunkSize);
				dataSeen = true;
			}
			offset = chunkData + chunkSize + (chunkSize & 1U);
		}
		return formatSeen && dataSeen
			&& parsed.dataOffset + parsed.dataSize <= bytes.size()
			&& parsed.dataSize % parsed.format.nBlockAlign == 0U;
	}
}

struct AudioPlayerBackend
{
	struct EffectVoice
	{
		IXAudio2SourceVoice* voice = nullptr;
		std::vector<std::uint8_t> wave;

		~EffectVoice()
		{
			if (voice != nullptr) voice->DestroyVoice();
		}
	};

	IXAudio2* engine = nullptr;
	IXAudio2MasteringVoice* masteringVoice = nullptr;
	IXAudio2SourceVoice* musicVoice = nullptr;
	std::vector<std::uint8_t> musicWave;
	std::vector<std::unique_ptr<EffectVoice>> effectVoices;

	~AudioPlayerBackend()
	{
		effectVoices.clear();
		if (musicVoice != nullptr)
		{
			musicVoice->DestroyVoice();
			musicVoice = nullptr;
		}
		if (masteringVoice != nullptr)
		{
			masteringVoice->DestroyVoice();
			masteringVoice = nullptr;
		}
		if (engine != nullptr)
		{
			engine->StopEngine();
			engine->Release();
			engine = nullptr;
		}
	}
};

AudioPlayer::AudioPlayer() = default;

AudioPlayer::~AudioPlayer()
{
	StopAll();
}

bool AudioPlayer::EnsureBackend() noexcept
{
	if (_backend != nullptr && _backend->engine != nullptr)
	{
		return true;
	}
	try
	{
		auto backend = std::make_unique<AudioPlayerBackend>();
		if (FAILED(::XAudio2Create(&backend->engine, 0, XAUDIO2_DEFAULT_PROCESSOR))
			|| backend->engine == nullptr
			|| FAILED(backend->engine->CreateMasteringVoice(&backend->masteringVoice)))
		{
			return false;
		}
		backend->engine->StartEngine();
		_backend = std::move(backend);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

void AudioPlayer::SetCatalog(AudioCatalog catalog)
{
	StopAll();
	_catalog = std::move(catalog);
}

void AudioPlayer::ApplyOptions(bool enabled, int effectsVolume, int musicVolume)
{
	const bool wasEnabled = _enabled;
	_enabled = enabled;
	_effectsVolume = std::clamp(effectsVolume, 0, 100);
	_musicVolume = std::clamp(musicVolume, 0, 100);
	if (!_enabled)
	{
		StopAll();
		return;
	}
	if (_musicVolume <= 0)
	{
		StopMusicImmediate();
		return;
	}
	if (wasEnabled && !_currentMusicCue.empty())
	{
		ApplyMusicVolume(_musicFade.GetCurrent());
	}
}

void AudioPlayer::ReclaimFinishedEffectVoices() noexcept
{
	if (_backend == nullptr) return;
	std::erase_if(_backend->effectVoices, [](const std::unique_ptr<AudioPlayerBackend::EffectVoice>& slot)
	{
		XAUDIO2_VOICE_STATE state{};
		slot->voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
		return state.BuffersQueued == 0U;
	});
}

void AudioPlayer::Update(float deltaSeconds)
{
	_effectGate.Update(deltaSeconds);
	ReclaimFinishedEffectVoices();
	_ducking.Update(deltaSeconds);
	if (!_enabled || _currentMusicCue.empty())
	{
		return;
	}

	const float level = _musicFade.Update(deltaSeconds, MUSIC_FADE_SECONDS);
	ApplyMusicVolume(level);
	if (!_pendingMusicCue.empty() && _musicFade.IsAtTarget() && level <= 0.001f)
	{
		const std::string nextCue = std::move(_pendingMusicCue);
		StopMusicImmediate();
		const AudioCueDefinition* cue = FindAudioCue(_catalog, nextCue);
		if (cue != nullptr && cue->kind == AudioCueKind::Music && BeginMusic(*cue, 0.0f))
		{
			_musicFade.Reset(0.0f);
			_musicFade.SetTarget(1.0f);
		}
	}
}

bool AudioPlayer::PlayEffect(std::string_view cueId)
{
	if (!_enabled || _effectsVolume <= 0)
	{
		return false;
	}
	const AudioCueDefinition* cue = FindAudioCue(_catalog, cueId);
	if (cue == nullptr || cue->kind != AudioCueKind::Effect || !_effectGate.TryAccept(cueId))
	{
		return false;
	}
	if (!EnsureBackend())
	{
		return false;
	}

	ReclaimFinishedEffectVoices();
	const AudioEffectCategory category = ClassifyAudioEffect(cueId);
	if (_backend->effectVoices.size() >= MAX_EFFECT_VOICES)
	{
		if (category == AudioEffectCategory::Interface || category == AudioEffectCategory::Peg)
		{
			return false;
		}
		_backend->effectVoices.erase(_backend->effectVoices.begin());
	}

	try
	{
		auto slot = std::make_unique<AudioPlayerBackend::EffectVoice>();
		if (!LoadWaveAsset(cue->filePath, slot->wave))
		{
			return false;
		}
		ParsedWave parsed;
		if (!ParseWave(slot->wave, parsed)
			|| FAILED(_backend->engine->CreateSourceVoice(&slot->voice, &parsed.format)))
		{
			return false;
		}
		XAUDIO2_BUFFER buffer{};
		buffer.Flags = XAUDIO2_END_OF_STREAM;
		buffer.AudioBytes = parsed.dataSize;
		buffer.pAudioData = slot->wave.data() + parsed.dataOffset;
		if (FAILED(slot->voice->SubmitSourceBuffer(&buffer))
			|| FAILED(slot->voice->SetVolume(static_cast<float>(_effectsVolume) / 100.0f))
			|| FAILED(slot->voice->Start()))
		{
			return false;
		}
		_backend->effectVoices.push_back(std::move(slot));
		_ducking.Trigger(category);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool AudioPlayer::StartMusic(std::string_view cueId)
{
	if (!_enabled || _musicVolume <= 0)
	{
		StopMusicImmediate();
		return false;
	}
	if (_currentMusicCue == cueId)
	{
		_pendingMusicCue.clear();
		_musicFade.SetTarget(1.0f);
		return true;
	}
	const AudioCueDefinition* cue = FindAudioCue(_catalog, cueId);
	if (cue == nullptr || cue->kind != AudioCueKind::Music)
	{
		return false;
	}
	if (!_currentMusicCue.empty())
	{
		_pendingMusicCue.assign(cueId);
		_musicFade.SetTarget(0.0f);
		return true;
	}
	_pendingMusicCue.clear();
	if (!BeginMusic(*cue, 0.0f))
	{
		return false;
	}
	_musicFade.Reset(0.0f);
	_musicFade.SetTarget(1.0f);
	return true;
}

bool AudioPlayer::BeginMusic(const AudioCueDefinition& cue, float initialLevel)
{
	if (!EnsureBackend())
	{
		return false;
	}
	std::vector<std::uint8_t> wave;
	if (!LoadWaveAsset(cue.filePath, wave))
	{
		return false;
	}
	ParsedWave parsed;
	if (!ParseWave(wave, parsed))
	{
		return false;
	}
	if (_backend->musicVoice != nullptr)
	{
		_backend->musicVoice->DestroyVoice();
		_backend->musicVoice = nullptr;
	}
	_backend->musicWave = std::move(wave);
	if (FAILED(_backend->engine->CreateSourceVoice(&_backend->musicVoice, &parsed.format)))
	{
		_backend->musicWave.clear();
		return false;
	}
	XAUDIO2_BUFFER buffer{};
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	buffer.AudioBytes = parsed.dataSize;
	buffer.pAudioData = _backend->musicWave.data() + parsed.dataOffset;
	buffer.LoopCount = cue.loop ? XAUDIO2_LOOP_INFINITE : 0U;
	if (FAILED(_backend->musicVoice->SubmitSourceBuffer(&buffer))
		|| FAILED(_backend->musicVoice->Start()))
	{
		_backend->musicVoice->DestroyVoice();
		_backend->musicVoice = nullptr;
		_backend->musicWave.clear();
		return false;
	}
	_currentMusicCue = cue.id;
	_lastAppliedMusicGain = -1.0f;
	ApplyMusicVolume(initialLevel);
	return true;
}

void AudioPlayer::ApplyMusicVolume(float level) noexcept
{
	if (_backend == nullptr || _backend->musicVoice == nullptr || _currentMusicCue.empty())
	{
		return;
	}
	const float gain = std::clamp(
		(static_cast<float>(_musicVolume) / 100.0f)
			* level
			* _ducking.GetGain(),
		0.0f,
		1.0f);
	if (std::fabs(gain - _lastAppliedMusicGain) <= 0.001f) return;
	_backend->musicVoice->SetVolume(gain);
	_lastAppliedMusicGain = gain;
}

void AudioPlayer::StopAll() noexcept
{
	if (_backend != nullptr)
	{
		_backend->effectVoices.clear();
	}
	_ducking.Reset();
	StopMusicImmediate();
}

void AudioPlayer::StopMusicImmediate() noexcept
{
	if (_backend != nullptr && _backend->musicVoice != nullptr)
	{
		_backend->musicVoice->DestroyVoice();
		_backend->musicVoice = nullptr;
		_backend->musicWave.clear();
	}
	_currentMusicCue.clear();
	_pendingMusicCue.clear();
	_musicFade.Reset(1.0f);
	_lastAppliedMusicGain = -1.0f;
}

std::size_t AudioPlayer::GetActiveEffectVoiceCount() const noexcept
{
	return _backend == nullptr ? 0U : _backend->effectVoices.size();
}
