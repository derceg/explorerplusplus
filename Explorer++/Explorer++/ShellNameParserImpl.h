// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellNameParser.h"

class ShellNameParserImpl : public ShellNameParser
{
public:
	ParseDisplayNameResult ParseDisplayName(const std::wstring &displayName) override;
};
