// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Clipboard.h"

class ClipboardStore;

// The Clipboard class won't clear the clipboard, unless explicitly told to.
// That makes sense, as it's possible to set multiple pieces of data on the
// clipboard, in different formats. The Clipboard class simply reads/writes
// data. It has no awareness of when the clipboard should be cleared.
// This class exists purely to make it slightly easier to write one (or several)
// pieces of data to the clipboard. It will clear the clipboard on construction,
// meaning that there's no need to do it manually (and no chance that it will be
// forgotten).
class BulkClipboardWriter
{
public:
	BulkClipboardWriter(ClipboardStore *clipboardStore);

	bool WriteText(const std::wstring &text);
	bool WriteCustomData(UINT format, const std::string &data);

private:
	Clipboard m_clipboard;
};
