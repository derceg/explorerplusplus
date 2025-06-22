// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ShellHelper.h"

PidlAbsolute CreateSimplePidlForTest(const std::wstring &path, IShellFolder *parent = nullptr,
	ShellItemType shellItemType = ShellItemType::Folder);
