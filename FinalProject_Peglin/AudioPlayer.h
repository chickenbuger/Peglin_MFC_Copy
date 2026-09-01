#pragma once

#include "AudioCatalog.h"

#include <string>
#include <string_view>
#include <vector>

class AudioPlayer
{
public:
	AudioPlayer() = default;
	~AudioPlayer();

	AudioPlayer(const AudioPlayer&) = delete;
	AudioPlayer& operator=(const AudioPlayer&) = delete;

	void SetCatalog(AudioCatalog catalog);
	void ApplyOptions(bool enabled, int effectsVolume, int musicVolume);
	bool PlayEffect(std::string_view cueId);
	bool StartMusic(std::string_view cueId);
	void StopAll() noexcept;
	bool IsCatalogReady() const noexcept { return !_catalog.cues.empty(); }
	std::string_view GetCurrentMusicCue() const noexcept { return _currentMusicCue; }

private:
	void StopMusic() noexcept;

	AudioCatalog _catalog;
	std::vector<std::uint8_t> _activeEffectWave;
	std::string _currentMusicCue;
	bool _enabled = true;
	int _effectsVolume = 70;
	int _musicVolume = 45;
};

