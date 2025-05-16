// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BulkClipboardWriter.h"

BulkClipboardWriter::BulkClipboardWriter(ClipboardStore *clipboardStore) :
	m_clipboard(clipboardStore)
{
	m_clipboard.Clear();
}

bool BulkClipboardWriter::WriteText(const std::wstring &text)
{
	return m_clipboard.WriteText(text);
}

bool BulkClipboardWriter::WriteCustomData(UINT format, const std::string &data)
{
	return m_clipboard.WriteCustomData(format, data);
}
