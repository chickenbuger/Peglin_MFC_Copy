#include "pch.h"
#include "SoftPegSound.h"

#include <algorithm>
#include <cmath>

namespace
{
	constexpr std::uint32_t SAMPLE_RATE = 22050;
	constexpr std::uint32_t DURATION_MILLISECONDS = 110;
	constexpr std::uint16_t CHANNEL_COUNT = 1;
	constexpr std::uint16_t BITS_PER_SAMPLE = 16;
	constexpr std::size_t HEADER_SIZE = 44;
	constexpr double PI = 3.14159265358979323846;

	void WriteUInt16(
		std::vector<std::uint8_t>& bytes,
		std::size_t offset,
		std::uint16_t value)
	{
		bytes[offset] = static_cast<std::uint8_t>(value & 0xffU);
		bytes[offset + 1] = static_cast<std::uint8_t>((value >> 8U) & 0xffU);
	}

	void WriteUInt32(
		std::vector<std::uint8_t>& bytes,
		std::size_t offset,
		std::uint32_t value)
	{
		for (std::size_t index = 0; index < 4; ++index)
		{
			bytes[offset + index] = static_cast<std::uint8_t>(
				(value >> static_cast<unsigned int>(index * 8U)) & 0xffU);
		}
	}
}

std::vector<std::uint8_t> CreateSoftPegHitWave()
{
	constexpr std::uint32_t sampleCount =
		SAMPLE_RATE * DURATION_MILLISECONDS / 1000U;
	constexpr std::uint32_t blockAlign = CHANNEL_COUNT * BITS_PER_SAMPLE / 8U;
	constexpr std::uint32_t dataSize = sampleCount * blockAlign;

	std::vector<std::uint8_t> wave(HEADER_SIZE + dataSize, 0U);
	wave[0] = 'R';
	wave[1] = 'I';
	wave[2] = 'F';
	wave[3] = 'F';
	WriteUInt32(wave, 4, 36U + dataSize);
	wave[8] = 'W';
	wave[9] = 'A';
	wave[10] = 'V';
	wave[11] = 'E';
	wave[12] = 'f';
	wave[13] = 'm';
	wave[14] = 't';
	wave[15] = ' ';
	WriteUInt32(wave, 16, 16U);
	WriteUInt16(wave, 20, 1U);
	WriteUInt16(wave, 22, CHANNEL_COUNT);
	WriteUInt32(wave, 24, SAMPLE_RATE);
	WriteUInt32(wave, 28, SAMPLE_RATE * blockAlign);
	WriteUInt16(wave, 32, static_cast<std::uint16_t>(blockAlign));
	WriteUInt16(wave, 34, BITS_PER_SAMPLE);
	wave[36] = 'd';
	wave[37] = 'a';
	wave[38] = 't';
	wave[39] = 'a';
	WriteUInt32(wave, 40, dataSize);

	for (std::uint32_t index = 0; index < sampleCount; ++index)
	{
		const double timeSeconds = static_cast<double>(index) / SAMPLE_RATE;
		const double attack = (std::min)(timeSeconds / 0.006, 1.0);
		const double decay = std::exp(-timeSeconds / 0.034);
		const double tailFade = 1.0
			- static_cast<double>(index) / static_cast<double>(sampleCount - 1U);
		const double tone = 0.72 * std::sin(2.0 * PI * 520.0 * timeSeconds)
			+ 0.28 * std::sin(2.0 * PI * 780.0 * timeSeconds);
		const double normalizedSample = 0.14 * attack * decay * tailFade * tone;
		const auto sample = static_cast<std::int16_t>(std::lround(
			(std::clamp)(normalizedSample, -1.0, 1.0) * 32767.0));
		const std::size_t offset = HEADER_SIZE
			+ static_cast<std::size_t>(index) * blockAlign;
		WriteUInt16(wave, offset, static_cast<std::uint16_t>(sample));
	}

	return wave;
}
