// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <ShObjIdl.h>
#include <string>

class PidlAbsolute;

enum class ShellItemType
{
	File,
	Folder
};

enum class ShellItemExtraAttributes
{
	None = 0,
	Hidden = FILE_ATTRIBUTE_HIDDEN
};

std::string EncodePidlToBase64(const PidlAbsolute &pidl);
PidlAbsolute DecodePidlFromBase64(const std::string &encodedPidl);

HRESULT CreateSimplePidl(const std::wstring &path, PidlAbsolute &outputPidl,
	IShellFolder *parent = nullptr, ShellItemType itemType = ShellItemType::File,
	ShellItemExtraAttributes extraAttributes = ShellItemExtraAttributes::None);
