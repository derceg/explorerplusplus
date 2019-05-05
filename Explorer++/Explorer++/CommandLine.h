// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/optional.hpp>

namespace CommandLine
{
	struct ExitInfo
	{
		int exitCode;
	};

	boost::optional<ExitInfo> ProcessCommandLine();
}