from __future__ import annotations

import math
import struct
import wave
from pathlib import Path


SAMPLE_RATE = 22_050
OUTPUT_DIRECTORY = Path(__file__).resolve().parents[1] / "FinalProject_Peglin" / "content" / "audio"


def envelope(time_seconds: float, duration: float, attack: float = 0.008) -> float:
    fade_in = min(time_seconds / attack, 1.0)
    fade_out = min((duration - time_seconds) / 0.05, 1.0)
    return max(0.0, min(fade_in, fade_out))


def write_wave(name: str, duration: float, sample_function) -> None:
    OUTPUT_DIRECTORY.mkdir(parents=True, exist_ok=True)
    frame_count = round(duration * SAMPLE_RATE)
    frames = bytearray()
    for index in range(frame_count):
        time_seconds = index / SAMPLE_RATE
        sample = max(-1.0, min(1.0, sample_function(time_seconds, duration)))
        frames.extend(struct.pack("<h", round(sample * 32767.0)))
    with wave.open(str(OUTPUT_DIRECTORY / name), "wb") as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(SAMPLE_RATE)
        output.writeframes(frames)


def tone(frequency: float, time_seconds: float) -> float:
    return math.sin(2.0 * math.pi * frequency * time_seconds)


def effect(name: str, duration: float, frequencies: tuple[float, ...], gain: float, decay: float) -> None:
    def sample(time_seconds: float, total: float) -> float:
        mixed = sum(tone(frequency, time_seconds) for frequency in frequencies) / len(frequencies)
        return gain * envelope(time_seconds, total) * math.exp(-time_seconds / decay) * mixed

    write_wave(name, duration, sample)


def loop(name: str, root: float, pattern: tuple[int, ...], gain: float) -> None:
    duration = 6.0

    def sample(time_seconds: float, total: float) -> float:
        beat = int(time_seconds * 2.0) % len(pattern)
        frequency = root * (2.0 ** (pattern[beat] / 12.0))
        pad = tone(frequency, time_seconds) + 0.45 * tone(frequency * 1.5, time_seconds)
        pulse = 0.72 + 0.28 * math.sin(2.0 * math.pi * 2.0 * time_seconds)
        seamless = math.sin(math.pi * min(time_seconds, total - time_seconds) / 0.08) if min(time_seconds, total - time_seconds) < 0.08 else 1.0
        return gain * pulse * seamless * pad / 1.45

    write_wave(name, duration, sample)


def main() -> None:
    effect("peg-hit.wav", 0.11, (520.0, 780.0), 0.14, 0.034)
    effect("bomb.wav", 0.32, (82.0, 118.0, 164.0), 0.24, 0.12)
    effect("refresh.wav", 0.34, (440.0, 660.0, 880.0), 0.18, 0.18)
    effect("attack.wav", 0.24, (320.0, 480.0), 0.18, 0.09)
    effect("damage.wav", 0.26, (145.0, 210.0), 0.22, 0.10)
    effect("victory.wav", 0.85, (392.0, 523.25, 659.25), 0.16, 0.55)
    effect("defeat.wav", 0.85, (196.0, 155.56, 130.81), 0.16, 0.50)
    effect("ui-confirm.wav", 0.12, (660.0, 990.0), 0.12, 0.05)
    loop("adventure-loop.wav", 110.0, (0, 3, 7, 3, 0, 5, 7, 5), 0.055)
    loop("battle-loop.wav", 146.83, (0, 0, 3, 0, 7, 5, 3, 5), 0.065)
    loop("shop-loop.wav", 130.81, (0, 7, 5, 3, 0, 3, 5, 7), 0.05)
    print(f"AUDIO ASSETS PASS: generated=11 directory={OUTPUT_DIRECTORY}")


if __name__ == "__main__":
    main()
