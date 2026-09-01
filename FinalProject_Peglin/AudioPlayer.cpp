#include "pch.h"
#include "AudioPlayer.h"

#include <algorithm>
#include <cmath>
#include <mmsystem.h>
#include <utility>

#pragma comment(lib, "Winmm.lib")

namespace
{
	constexpr wchar_t MUSIC_ALIAS[] = L"peglin_background_music";
	constexpr float MUSIC_FADE_SECONDS = 0.28f;

	std::wstring QuotePath(const std::filesystem::path& path)
	{
		return L"\"" + path.wstring() + L"\"";
	}
}

AudioPlayer::~AudioPlayer()
{
	StopAll();
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
	if (wasEnabled && !_currentMusicCue.empty()) ApplyMusicVolume(_musicFade.GetCurrent());
}

void AudioPlayer::Update(float deltaSeconds)
{
	_effectGate.Update(deltaSeconds);
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
	if (cue == nullptr || cue->kind != AudioCueKind::Effect)
	{
		return false;
	}
	if (!_effectGate.TryAccept(cueId))
	{
		return false;
	}

	std::vector<std::uint8_t> source;
	if (!LoadWaveAsset(cue->filePath, source))
	{
		return false;
	}
	_activeEffectWave = ScalePcm16Wave(source, _effectsVolume);
	if (_activeEffectWave.empty())
	{
		return false;
	}
	return ::PlaySoundW(
		reinterpret_cast<LPCWSTR>(_activeEffectWave.data()),
		nullptr,
		SND_MEMORY | SND_ASYNC | SND_NODEFAULT) != FALSE;
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
	std::wstring openCommand = L"open ";
	openCommand += QuotePath(cue.filePath);
	openCommand += L" type waveaudio alias ";
	openCommand += MUSIC_ALIAS;
	if (::mciSendStringW(openCommand.c_str(), nullptr, 0, nullptr) != 0)
	{
		return false;
	}

	_currentMusicCue = cue.id;
	_lastAppliedMusicVolume = -1;
	ApplyMusicVolume(initialLevel);

	std::wstring playCommand = L"play ";
	playCommand += MUSIC_ALIAS;
	if (cue.loop) playCommand += L" repeat";
	if (::mciSendStringW(playCommand.c_str(), nullptr, 0, nullptr) != 0)
	{
		StopMusicImmediate();
		return false;
	}
	return true;
}

void AudioPlayer::ApplyMusicVolume(float level) noexcept
{
	const int volume = std::clamp(
		static_cast<int>(std::lround(static_cast<float>(_musicVolume * 10) * level)),
		0,
		1000);
	if (volume == _lastAppliedMusicVolume || _currentMusicCue.empty()) return;
	std::wstring command = L"setaudio ";
	command += MUSIC_ALIAS;
	command += L" volume to ";
	command += std::to_wstring(volume);
	::mciSendStringW(command.c_str(), nullptr, 0, nullptr);
	_lastAppliedMusicVolume = volume;
}

void AudioPlayer::StopAll() noexcept
{
	::PlaySoundW(nullptr, nullptr, 0);
	_activeEffectWave.clear();
	StopMusicImmediate();
}

void AudioPlayer::StopMusicImmediate() noexcept
{
	std::wstring command = L"stop ";
	command += MUSIC_ALIAS;
	::mciSendStringW(command.c_str(), nullptr, 0, nullptr);
	command = L"close ";
	command += MUSIC_ALIAS;
	::mciSendStringW(command.c_str(), nullptr, 0, nullptr);
	_currentMusicCue.clear();
	_pendingMusicCue.clear();
	_musicFade.Reset(1.0f);
	_lastAppliedMusicVolume = -1;
}
