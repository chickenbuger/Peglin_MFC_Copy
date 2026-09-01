#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

enum class CombatLogTone
{
	Neutral,
	Positive,
	Warning,
	Danger
};

struct CombatLogEntry
{
	std::size_t sequence = 0;
	std::wstring text;
	CombatLogTone tone = CombatLogTone::Neutral;
};

class CombatLog
{
public:
	explicit CombatLog(std::size_t capacity = 12) noexcept
		: _capacity((std::max)(std::size_t{ 1 }, capacity))
	{
	}

	void Clear() noexcept
	{
		_entries.clear();
		_nextSequence = 1;
	}

	void Add(std::wstring text, CombatLogTone tone = CombatLogTone::Neutral)
	{
		if (text.empty())
		{
			return;
		}
		if (_entries.size() == _capacity)
		{
			_entries.erase(_entries.begin());
		}
		_entries.push_back({ _nextSequence++, std::move(text), tone });
	}

	std::size_t GetCapacity() const noexcept { return _capacity; }
	const std::vector<CombatLogEntry>& GetEntries() const noexcept { return _entries; }
	bool IsEmpty() const noexcept { return _entries.empty(); }

private:
	std::size_t _capacity = 12;
	std::size_t _nextSequence = 1;
	std::vector<CombatLogEntry> _entries;
};
