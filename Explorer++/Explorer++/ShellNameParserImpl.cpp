// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellNameParserImpl.h"

ShellNameParser::ParseDisplayNameResult ShellNameParserImpl::ParseDisplayName(
	const std::wstring &displayName)
{
	PidlAbsolute pidl;
	RETURN_IF_FAILED(
		SHParseDisplayName(displayName.c_str(), nullptr, PidlOutParam(pidl), 0, nullptr));
	return pidl;
}
