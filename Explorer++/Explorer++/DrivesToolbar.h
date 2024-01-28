// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/FileContextMenuManager.h"
#include <memory>
#include <string>

class BrowserWindow;
class CoreInterface;
class DriveModel;
class DrivesToolbarView;
struct MouseEvent;

class DrivesToolbar : private FileContextMenuHandler
{
public:
	static DrivesToolbar *Create(DrivesToolbarView *view, std::unique_ptr<DriveModel> driveModel,
		BrowserWindow *browserWindow, CoreInterface *coreInterface);

	DrivesToolbar(const DrivesToolbar &) = delete;
	DrivesToolbar(DrivesToolbar &&) = delete;
	DrivesToolbar &operator=(const DrivesToolbar &) = delete;
	DrivesToolbar &operator=(DrivesToolbar &&) = delete;

	DrivesToolbarView *GetView() const;

private:
	static constexpr int MIN_SHELL_MENU_ID = 1;
	static constexpr int MAX_SHELL_MENU_ID = 1000;

	static constexpr int MENU_ID_OPEN_IN_NEW_TAB = (MAX_SHELL_MENU_ID + 1);

	DrivesToolbar(DrivesToolbarView *view, std::unique_ptr<DriveModel> driveModel,
		BrowserWindow *browserWindow, CoreInterface *coreInterface);
	~DrivesToolbar();

	void Initialize();

	void AddDrives();
	void AddDriveAtIndex(const std::wstring &drivePath, size_t index);

	void OnDriveAdded(const std::wstring &path, size_t index);
	void OnDriveUpdated(const std::wstring &path);
	void OnDriveRemoved(const std::wstring &path, size_t oldIndex);

	void OnButtonClicked(const std::wstring &drivePath, const MouseEvent &event);
	void OnButtonMiddleClicked(const std::wstring &drivePath, const MouseEvent &event);
	void OnButtonRightClicked(const std::wstring &drivePath, const MouseEvent &event);

	void ShowContextMenu(const std::wstring &drivePath, const POINT &ptClient, bool showExtended);

	// FileContextMenuHandler
	void UpdateMenuEntries(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, IContextMenu *contextMenu,
		HMENU hMenu) override;
	BOOL HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, const TCHAR *szCmd) override;
	void HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PITEMID_CHILD> &pidlItems, int iCmd) override;

	void OnWindowDestroyed();

	DrivesToolbarView *m_view = nullptr;
	std::unique_ptr<DriveModel> m_driveModel;
	BrowserWindow *m_browserWindow = nullptr;
	CoreInterface *m_coreInterface = nullptr;
};
