// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Navigator.h"
#include <boost/core/noncopyable.hpp>

class BrowserWindow;
class ShellBrowser;

class BrowserCommandController : private boost::noncopyable
{
public:
	BrowserCommandController(BrowserWindow *browserWindow);

	void ExecuteCommand(int command,
		OpenFolderDisposition disposition = OpenFolderDisposition::CurrentTab);

private:
	void GoBack(OpenFolderDisposition disposition);
	void GoForward(OpenFolderDisposition disposition);
	void GoUp(OpenFolderDisposition disposition);
	void GoToPath(const std::wstring &path, OpenFolderDisposition disposition);
	void GoToKnownFolder(REFKNOWNFOLDERID knownFolderId, OpenFolderDisposition disposition);

	ShellBrowser *GetActiveShellBrowser() const;

	BrowserWindow *const m_browserWindow;
};
