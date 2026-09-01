#include "pch.h"
#include "AudioPlayer.h"

#include <algorithm>
#include <mmsystem.h>
#include <utility>

#pragma comment(lib, "Winmm.lib")

namespace
{
	constexpr wchar_t MUSIC_ALIAS[] = L"peglin_background_music";

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
	if (wasEnabled && !_currentMusicCue.empty())
	{
		std::wstring volumeCommand = L"setaudio ";
		volumeCommand += MUSIC_ALIAS;
		volumeCommand += L" volume to ";
		volumeCommand += std::to_wstring(_musicVolume * 10);
		::mciSendStringW(volumeCommand.c_str(), nullptr, 0, nullptr);
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
		StopMusic();
		return false;
	}
	if (_currentMusicCue == cueId)
	{
		return true;
	}
	const AudioCueDefinition* cue = FindAudioCue(_catalog, cueId);
	if (cue == nullptr || cue->kind != AudioCueKind::Music)
	{
		return false;
	}

	StopMusic();
	std::wstring openCommand = L"open ";
	openCommand += QuotePath(cue->filePath);
	openCommand += L" type waveaudio alias ";
	openCommand += MUSIC_ALIAS;
	if (::mciSendStringW(openCommand.c_str(), nullptr, 0, nullptr) != 0)
	{
		return false;
	}

	std::wstring volumeCommand = L"setaudio ";
	volumeCommand += MUSIC_ALIAS;
	volumeCommand += L" volume to ";
	volumeCommand += std::to_wstring(_musicVolume * 10);
	::mciSendStringW(volumeCommand.c_str(), nullptr, 0, nullptr);

	std::wstring playCommand = L"play ";
	playCommand += MUSIC_ALIAS;
	if (cue->loop) playCommand += L" repeat";
	if (::mciSendStringW(playCommand.c_str(), nullptr, 0, nullptr) != 0)
	{
		StopMusic();
		return false;
	}
	_currentMusicCue = cue->id;
	return true;
}

void AudioPlayer::StopAll() noexcept
{
	::PlaySoundW(nullptr, nullptr, 0);
	_activeEffectWave.clear();
	StopMusic();
}

void AudioPlayer::StopMusic() noexcept
{
	std::wstring command = L"stop ";
	command += MUSIC_ALIAS;
	::mciSendStringW(command.c_str(), nullptr, 0, nullptr);
	command = L"close ";
	command += MUSIC_ALIAS;
	::mciSendStringW(command.c_str(), nullptr, 0, nullptr);
	_currentMusicCue.clear();
}
