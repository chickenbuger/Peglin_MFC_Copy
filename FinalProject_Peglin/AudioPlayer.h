#pragma once

#include "AudioCatalog.h"
#include "AudioMixing.h"

#include <string>
#include <string_view>
#include <memory>

struct AudioPlayerBackend;

class AudioPlayer
{
public:
	AudioPlayer();
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
	std::size_t GetActiveEffectVoiceCount() const noexcept;

private:
	bool BeginMusic(const AudioCueDefinition& cue, float initialLevel);
	void ApplyMusicVolume(float level) noexcept;
	void StopMusicImmediate() noexcept;
	bool EnsureBackend() noexcept;
	void ReclaimFinishedEffectVoices() noexcept;

	AudioCatalog _catalog;
	std::unique_ptr<AudioPlayerBackend> _backend;
	std::string _currentMusicCue;
	std::string _pendingMusicCue;
	AudioEffectGate _effectGate;
	AudioFadeEnvelope _musicFade;
	AudioDuckingEnvelope _ducking;
	bool _enabled = true;
	int _effectsVolume = 70;
	int _musicVolume = 45;
	float _lastAppliedMusicGain = -1.0f;
};
