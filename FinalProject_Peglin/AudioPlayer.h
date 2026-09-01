#pragma once

#include "AudioCatalog.h"
#include "AudioMixing.h"

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
	void Update(float deltaSeconds);
	bool PlayEffect(std::string_view cueId);
	bool StartMusic(std::string_view cueId);
	void StopAll() noexcept;
	bool IsCatalogReady() const noexcept { return !_catalog.cues.empty(); }
	std::string_view GetCurrentMusicCue() const noexcept { return _currentMusicCue; }

private:
	bool BeginMusic(const AudioCueDefinition& cue, float initialLevel);
	void ApplyMusicVolume(float level) noexcept;
	void StopMusicImmediate() noexcept;

	AudioCatalog _catalog;
	std::vector<std::uint8_t> _activeEffectWave;
	std::string _currentMusicCue;
	std::string _pendingMusicCue;
	AudioEffectGate _effectGate;
	AudioFadeEnvelope _musicFade;
	bool _enabled = true;
	int _effectsVolume = 70;
	int _musicVolume = 45;
	int _lastAppliedMusicVolume = -1;
};
