// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "ClipboardOperations.h"
#include "DirectoryOperationsHelper.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellTreeView/ShellTreeView.h"

BOOL Explorerplusplus::CanPaste(PasteType pasteType) const
{
	auto directory = MaybeGetFocusedDirectory();

	if (!directory.HasValue())
	{
		return false;
	}

	return CanPasteInDirectory(m_app->GetPlatformContext()->GetClipboardStore(), directory.Raw(),
		pasteType);
}

// Tests whether a hard link or symlink can be pasted.
bool Explorerplusplus::CanPasteLink() const
{
	const auto *activeShellBrowser = GetActiveShellBrowserImpl();
	return ClipboardOperations::CanPasteLinkInDirectory(
		m_app->GetPlatformContext()->GetClipboardStore(),
		activeShellBrowser->GetDirectoryIdl().get());
}

PidlAbsolute Explorerplusplus::MaybeGetFocusedDirectory() const
{
	HWND focus = GetFocus();

	if (!focus)
	{
		return nullptr;
	}

	unique_pidl_absolute directory;

	const auto *activeShellBrowser = GetActiveShellBrowserImpl();

	if (focus == activeShellBrowser->GetListView())
	{
		directory = activeShellBrowser->GetDirectoryIdl();
	}
	else if (focus == m_shellTreeView->GetHWND())
	{
		directory = m_shellTreeView->GetSelectedNodePidl();
	}

	return directory.get();
}
