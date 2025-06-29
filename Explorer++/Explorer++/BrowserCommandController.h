// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Literals.h"
#include "NavigationHelper.h"
#include "ShellBrowser/SortModes.h"
#include "../Helper/ShellHelper.h"
#include <boost/core/noncopyable.hpp>

class BrowserWindow;
class ClipboardStore;
struct Config;
class ResourceLoader;
class ShellBrowser;

class BrowserCommandController : private boost::noncopyable
{
public:
	BrowserCommandController(BrowserWindow *browser, Config *config, ClipboardStore *clipboardStore,
		const ResourceLoader *resourceLoader);

	bool IsCommandEnabled(int command) const;
	void ExecuteCommand(int command,
		OpenFolderDisposition disposition = OpenFolderDisposition::CurrentTab);

private:
	static constexpr wchar_t DOCUMENTATION_URL[] =
		L"https://explorerplusplus.readthedocs.io/en/latest/";

	// When changing the font size, it will be decreased/increased by this amount.
	static constexpr int FONT_SIZE_CHANGE_DELTA = 1_pt;

	enum class FontSizeType
	{
		Decrease,
		Increase
	};

	bool IsCommandContextSensitive(int command) const;

	bool CanStartCommandPrompt() const;
	bool CanChangeMainFontSize(FontSizeType sizeType) const;

	void OnSortBy(SortMode sortMode);
	void OnCloseTab();
	void StartCommandPrompt(LaunchProcessFlags flags = LaunchProcessFlags::None);
	void CopyFolderPath() const;
	void OnChangeMainFontSize(FontSizeType sizeType);
	void OnResetMainFontSize();
	void OnChangeDisplayColors();
	void GoBack(OpenFolderDisposition disposition);
	void GoForward(OpenFolderDisposition disposition);
	void GoUp(OpenFolderDisposition disposition);
	void GoToPath(const std::wstring &path, OpenFolderDisposition disposition);
	void GoToKnownFolder(REFKNOWNFOLDERID knownFolderId, OpenFolderDisposition disposition);
	void OnOpenOnlineDocumentation();
	void OnCheckForUpdates();
	void OnAbout();
	void OnDuplicateTab();
	void OnSelectTabAtIndex(int index);
	void OnSelectLastTab();

	ShellBrowser *GetActiveShellBrowser();
	const ShellBrowser *GetActiveShellBrowser() const;

	BrowserWindow *const m_browser;
	Config *const m_config;
	ClipboardStore *const m_clipboardStore;
	const ResourceLoader *const m_resourceLoader;
};
