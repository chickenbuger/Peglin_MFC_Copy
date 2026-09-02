#include "pch.h"
#include "AudioCatalog.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iterator>
#include <limits>
#include <set>
#include <system_error>

namespace
{
	constexpr std::size_t MAX_CATALOG_BYTES = 64U * 1024U;
	constexpr std::size_t MAX_WAVE_BYTES = 4U * 1024U * 1024U;
	constexpr std::string_view VERSION_LINE = "peglin_audio_version=1";

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

	void WriteUInt16(std::vector<std::uint8_t>& bytes, std::size_t offset, std::int16_t value)
	{
		const auto sample = static_cast<std::uint16_t>(value);
		bytes[offset] = static_cast<std::uint8_t>(sample & 0xffU);
		bytes[offset + 1] = static_cast<std::uint8_t>((sample >> 8U) & 0xffU);
	}

	bool IsSafeId(std::string_view id) noexcept
	{
		return !id.empty() && id.size() <= 48U
			&& std::all_of(id.begin(), id.end(), [](char character)
			{
				return (character >= 'a' && character <= 'z')
					|| (character >= '0' && character <= '9')
					|| character == '_' || character == '-';
			});
	}

	bool IsSafeRelativeWavePath(const std::filesystem::path& path) noexcept
	{
		if (path.empty() || path.is_absolute() || path.has_root_path() || path.extension() != L".wav")
		{
			return false;
		}
		for (const auto& component : path)
		{
			if (component == L".." || component == L".")
			{
				return false;
			}
		}
		return true;
	}

	std::vector<std::string_view> Split(std::string_view value)
	{
		std::vector<std::string_view> fields;
		std::size_t begin = 0;
		while (begin <= value.size())
		{
			const std::size_t separator = value.find('|', begin);
			if (separator == std::string_view::npos)
			{
				fields.push_back(value.substr(begin));
				break;
			}
			fields.push_back(value.substr(begin, separator - begin));
			begin = separator + 1;
		}
		return fields;
	}

	AudioCatalogLoadResult Failure(
		AudioCatalogLoadState state,
		AudioCatalogLoadError error,
		std::size_t line,
		std::string message)
	{
		AudioCatalogLoadResult result;
		result.state = state;
		result.error = error;
		result.line = line;
		result.message = std::move(message);
		return result;
	}

	bool FindPcmData(
		const std::vector<std::uint8_t>& bytes,
		std::size_t& dataOffset,
		std::size_t& dataSize) noexcept
	{
		if (bytes.size() < 44U
			|| std::string_view(reinterpret_cast<const char*>(bytes.data()), 4) != "RIFF"
			|| std::string_view(reinterpret_cast<const char*>(bytes.data() + 8), 4) != "WAVE")
		{
			return false;
		}

		bool validFormat = false;
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
				const std::uint16_t encoding = ReadUInt16(bytes, chunkData);
				const std::uint16_t channels = ReadUInt16(bytes, chunkData + 2U);
				const std::uint16_t bits = ReadUInt16(bytes, chunkData + 14U);
				validFormat = encoding == 1U && (channels == 1U || channels == 2U) && bits == 16U;
			}
			else if (id == "data")
			{
				dataOffset = chunkData;
				dataSize = chunkSize;
			}
			offset = chunkData + chunkSize + (chunkSize & 1U);
		}
		return validFormat && dataSize > 0U && dataSize % 2U == 0U;
	}
}

AudioCatalogLoadResult LoadAudioCatalog(const std::filesystem::path& path) noexcept
{
	try
	{
		std::error_code existsError;
		if (!std::filesystem::exists(path, existsError))
		{
			return Failure(
				existsError ? AudioCatalogLoadState::IoError : AudioCatalogLoadState::Missing,
				existsError ? AudioCatalogLoadError::IoFailure : AudioCatalogLoadError::MissingFile,
				0,
				existsError ? existsError.message() : "audio catalog is missing");
		}

		std::error_code sizeError;
		const std::uintmax_t byteCount = std::filesystem::file_size(path, sizeError);
		if (sizeError)
		{
			return Failure(AudioCatalogLoadState::IoError, AudioCatalogLoadError::IoFailure, 0, sizeError.message());
		}
		if (byteCount > MAX_CATALOG_BYTES)
		{
			return Failure(AudioCatalogLoadState::Invalid, AudioCatalogLoadError::FileTooLarge, 0, "audio catalog is too large");
		}

		std::ifstream stream(path, std::ios::binary);
		if (!stream)
		{
			return Failure(AudioCatalogLoadState::IoError, AudioCatalogLoadError::IoFailure, 0, "audio catalog could not be opened");
		}

		AudioCatalogLoadResult result;
		result.state = AudioCatalogLoadState::Loaded;
		std::set<std::string> cueIds;
		std::string line;
		std::size_t lineNumber = 0;
		bool versionSeen = false;
		while (std::getline(stream, line))
		{
			++lineNumber;
			if (!line.empty() && line.back() == '\r') line.pop_back();
			if (line.empty() || line.starts_with('#')) continue;
			if (!versionSeen)
			{
				if (line != VERSION_LINE)
				{
					return Failure(AudioCatalogLoadState::Invalid, AudioCatalogLoadError::UnsupportedVersion, lineNumber, "audio catalog version is invalid");
				}
				versionSeen = true;
				continue;
			}
			if (!line.starts_with("cue="))
			{
				return Failure(AudioCatalogLoadState::Invalid, AudioCatalogLoadError::MalformedLine, lineNumber, "audio catalog line is malformed");
			}

			const std::vector<std::string_view> fields = Split(std::string_view(line).substr(4));
			if (fields.size() != 4U || !IsSafeId(fields[0]))
			{
				return Failure(AudioCatalogLoadState::Invalid, AudioCatalogLoadError::InvalidCue, lineNumber, "audio cue is invalid");
			}
			if (!cueIds.emplace(fields[0]).second)
			{
				return Failure(AudioCatalogLoadState::Invalid, AudioCatalogLoadError::DuplicateCue, lineNumber, "audio cue is duplicated");
			}

			AudioCueDefinition cue;
			cue.id = fields[0];
			cue.kind = fields[1] == "effect" ? AudioCueKind::Effect : AudioCueKind::Music;
			if (fields[1] != "effect" && fields[1] != "music")
			{
				return Failure(AudioCatalogLoadState::Invalid, AudioCatalogLoadError::InvalidCue, lineNumber, "audio cue kind is invalid");
			}
			const std::filesystem::path relativePath{ std::string(fields[2]) };
			if (!IsSafeRelativeWavePath(relativePath))
			{
				return Failure(AudioCatalogLoadState::Invalid, AudioCatalogLoadError::UnsafePath, lineNumber, "audio cue path is unsafe");
			}
			if (fields[3] == "1") cue.loop = true;
			else if (fields[3] == "0") cue.loop = false;
			else return Failure(AudioCatalogLoadState::Invalid, AudioCatalogLoadError::InvalidCue, lineNumber, "audio loop value is invalid");

			cue.filePath = (path.parent_path() / relativePath).lexically_normal();
			std::vector<std::uint8_t> wave;
			if (!LoadWaveAsset(cue.filePath, wave))
			{
				std::error_code assetExistsError;
				const bool exists = std::filesystem::exists(cue.filePath, assetExistsError);
				return Failure(
					assetExistsError ? AudioCatalogLoadState::IoError : AudioCatalogLoadState::Invalid,
					!exists ? AudioCatalogLoadError::MissingAsset : AudioCatalogLoadError::InvalidWave,
					lineNumber,
					!exists ? "audio asset is missing" : "audio asset is not supported PCM WAV");
			}
			result.catalog.cues.push_back(std::move(cue));
		}

		if (!versionSeen || result.catalog.cues.empty())
		{
			return Failure(AudioCatalogLoadState::Invalid, AudioCatalogLoadError::InvalidCue, lineNumber, "audio catalog has no cues");
		}
		return result;
	}
	catch (const std::exception& exception)
	{
		return Failure(AudioCatalogLoadState::IoError, AudioCatalogLoadError::IoFailure, 0, exception.what());
	}
}

const AudioCueDefinition* FindAudioCue(const AudioCatalog& catalog, std::string_view id) noexcept
{
	const auto cue = std::find_if(catalog.cues.begin(), catalog.cues.end(), [id](const AudioCueDefinition& candidate)
	{
		return candidate.id == id;
	});
	return cue == catalog.cues.end() ? nullptr : &*cue;
}

bool LoadWaveAsset(
	const std::filesystem::path& path,
	std::vector<std::uint8_t>& bytes,
	std::string* errorMessage) noexcept
{
	try
	{
		std::error_code sizeError;
		const std::uintmax_t size = std::filesystem::file_size(path, sizeError);
		if (sizeError || size < 44U || size > MAX_WAVE_BYTES)
		{
			if (errorMessage != nullptr) *errorMessage = sizeError ? sizeError.message() : "wave size is invalid";
			return false;
		}
		std::ifstream stream(path, std::ios::binary);
		if (!stream)
		{
			if (errorMessage != nullptr) *errorMessage = "wave file could not be opened";
			return false;
		}
		bytes.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
		std::size_t dataOffset = 0;
		std::size_t dataSize = 0;
		if (!FindPcmData(bytes, dataOffset, dataSize))
		{
			bytes.clear();
			if (errorMessage != nullptr) *errorMessage = "wave must be PCM16 mono or stereo";
			return false;
		}
		if (errorMessage != nullptr) errorMessage->clear();
		return true;
	}
	catch (const std::exception& exception)
	{
		bytes.clear();
		if (errorMessage != nullptr) *errorMessage = exception.what();
		return false;
	}
}

std::vector<std::uint8_t> ScalePcm16Wave(
	const std::vector<std::uint8_t>& source,
	int volumePercent)
{
	std::size_t dataOffset = 0;
	std::size_t dataSize = 0;
	if (!FindPcmData(source, dataOffset, dataSize))
	{
		return {};
	}

	std::vector<std::uint8_t> scaled = source;
	const float multiplier = static_cast<float>(std::clamp(volumePercent, 0, 100)) / 100.0f;
	for (std::size_t offset = dataOffset; offset < dataOffset + dataSize; offset += 2U)
	{
		const auto raw = static_cast<std::uint16_t>(scaled[offset])
			| static_cast<std::uint16_t>(scaled[offset + 1]) << 8U;
		const auto sample = static_cast<std::int16_t>(raw);
		const auto adjusted = static_cast<std::int16_t>(std::lround(static_cast<float>(sample) * multiplier));
		WriteUInt16(scaled, offset, adjusted);
	}
	return scaled;
}

std::filesystem::path GetDefaultAudioCatalogPath() noexcept
{
	try
	{
		std::array<wchar_t, 32768> modulePath{};
		const DWORD length = ::GetModuleFileNameW(nullptr, modulePath.data(), static_cast<DWORD>(modulePath.size()));
		if (length > 0 && length < modulePath.size())
		{
			return std::filesystem::path(modulePath.data()).parent_path() / L"content" / L"audio.v1.ini";
		}
	}
	catch (...)
	{
	}
	return std::filesystem::path(L"content") / L"audio.v1.ini";
}
