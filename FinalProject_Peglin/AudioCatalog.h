#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

enum class AudioCueKind
{
	Effect,
	Music
};

struct AudioCueDefinition
{
	std::string id;
	std::filesystem::path filePath;
	AudioCueKind kind = AudioCueKind::Effect;
	bool loop = false;
};

struct AudioCatalog
{
	std::vector<AudioCueDefinition> cues;
};

enum class AudioCatalogLoadState
{
	Loaded,
	Missing,
	Invalid,
	IoError
};

enum class AudioCatalogLoadError
{
	None,
	MissingFile,
	IoFailure,
	FileTooLarge,
	InvalidEncoding,
	UnsupportedVersion,
	MalformedLine,
	DuplicateCue,
	InvalidCue,
	UnsafePath,
	MissingAsset,
	InvalidWave
};

struct AudioCatalogLoadResult
{
	AudioCatalog catalog;
	AudioCatalogLoadState state = AudioCatalogLoadState::Missing;
	AudioCatalogLoadError error = AudioCatalogLoadError::None;
	std::size_t line = 0;
	std::string message;

	bool IsUsable() const noexcept
	{
		return state == AudioCatalogLoadState::Loaded && error == AudioCatalogLoadError::None;
	}
};

AudioCatalogLoadResult LoadAudioCatalog(const std::filesystem::path& path) noexcept;
const AudioCueDefinition* FindAudioCue(const AudioCatalog& catalog, std::string_view id) noexcept;
bool LoadWaveAsset(
	const std::filesystem::path& path,
	std::vector<std::uint8_t>& bytes,
	std::string* errorMessage = nullptr) noexcept;
std::vector<std::uint8_t> ScalePcm16Wave(
	const std::vector<std::uint8_t>& source,
	int volumePercent);
std::filesystem::path GetDefaultAudioCatalogPath() noexcept;

