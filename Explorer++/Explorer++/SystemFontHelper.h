// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

enum class SystemFont
{
	Caption,
	SmallCaption,
	Menu,
	Status,
	Message
};

LOGFONT GetSystemFont(SystemFont systemFont, HWND hwnd);
LOGFONT GetDefaultSystemFont(HWND hwnd);
