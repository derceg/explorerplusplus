// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellNameParserFake.h"
#include "PidlTestHelper.h"

ShellNameParser::ParseDisplayNameResult ShellNameParserFake::ParseDisplayName(
	const std::wstring &displayName)
{
	return CreateSimplePidlForTest(displayName);
}
