// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/outcome.hpp>
#include <set>
#include <string>

namespace outcome = BOOST_OUTCOME_V2_NAMESPACE;

class DriveEnumerator
{
public:
	virtual ~DriveEnumerator() = default;

	virtual outcome::std_result<std::set<std::wstring>> GetDrives() = 0;
};
