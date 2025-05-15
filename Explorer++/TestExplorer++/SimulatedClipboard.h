// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/Clipboard.h"
#include <cstddef>
#include <unordered_map>
#include <vector>

// Using the system clipboard in a unit test isn't ideal, for at least two reasons:
//
// 1. Other applications can sometimes have the clipboard open, causing the test to intermittently
//    fail.
// 2. It can be hard to ensure that the clipboard remains open for the duration of the test. The
//    code being tested might open the clipboard, write data, then close the clipboard, making it
//    hard for the unit test to guarantee that no other application will write data before it can be
//    read.
//
// Also, the simulated clipboard has the small advantage that it always starts empty, so there's no
// risk of forgetting to clear it at the start of a test.
//
// This class has a small amount of functionality, just enough to allow unit tests to work. It's not
// designed to be a complete recreation of the system clipboard.
class SimulatedClipboard : public Clipboard
{
public:
	std::optional<std::wstring> ReadText() override;
	std::optional<std::string> ReadCustomData(UINT format) override;

	bool WriteText(const std::wstring &text) override;
	bool WriteCustomData(UINT format, const std::string &data) override;

	bool Clear() override;

private:
	template <typename Char>
	std::optional<std::basic_string<Char>> ReadData(UINT format) const
	{
		auto itr = m_data.find(format);

		if (itr == m_data.end())
		{
			return std::nullopt;
		}

		CHECK((itr->second.size() % sizeof(Char)) == 0);

		return std::basic_string(reinterpret_cast<const Char *>(itr->second.data()),
			itr->second.size() / sizeof(Char));
	}

	template <typename Char>
	void WriteData(UINT format, const std::basic_string<Char> &data)
	{
		m_data[format] = std::vector<std::byte>(reinterpret_cast<const std::byte *>(data.data()),
			reinterpret_cast<const std::byte *>(data.data()) + (data.size() * sizeof(Char)));
	}

	std::unordered_map<UINT, std::vector<std::byte>> m_data;
};
