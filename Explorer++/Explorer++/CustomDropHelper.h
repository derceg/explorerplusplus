// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ClipboardHelper.h"
#include <shtypes.h>
#include <optional>
#include <string>

class ClipboardStore;

namespace CustomDropHelper
{

enum class DataType
{
	Text,
	PngImage
};

struct FileContents
{
	DataType type;
	std::string contents;
};

bool CanPasteInDirectory(ClipboardStore *clipboardStore, PCIDLIST_ABSOLUTE pidl,
	PasteType pasteType);
std::optional<FileContents> MaybeExtractData(ClipboardStore *clipboardStore);

}
