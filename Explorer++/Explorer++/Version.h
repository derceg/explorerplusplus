// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <string>
#include <vector>

class Version
{
public:
	Version(const std::initializer_list<uint32_t> &segments);

	// The comparison operators for std::vector will do the correct thing here. That is, the version
	// vectors will be compared lexicographically.
	auto operator<=>(const Version &) const = default;

	const std::vector<uint32_t> &GetSegments() const;
	std::wstring GetString() const;

private:
	std::vector<uint32_t> m_segments;
};
