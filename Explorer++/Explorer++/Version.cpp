// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Version.h"
#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>

Version::Version(const std::initializer_list<uint32_t> &segments) : m_segments(segments)
{
}

const std::vector<uint32_t> &Version::GetSegments() const
{
	return m_segments;
}

std::wstring Version::GetString() const
{
	return boost::algorithm::join(m_segments
			| boost::adaptors::transformed(
				[](uint32_t segment) { return std::to_wstring(segment); }),
		L".");
}
