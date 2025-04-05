// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ShellContextMenu.h"
#include <memory>
#include <string>

class BrowserWindow;
class DriveModel;
class DrivesToolbarView;
struct MouseEvent;
class ResourceLoader;

class DrivesToolbar : private ShellContextMenuHandler
{
public:
	static DrivesToolbar *Create(DrivesToolbarView *view, std::unique_ptr<DriveModel> driveModel,
		BrowserWindow *browserWindow, const ResourceLoader *resourceLoader);

	DrivesToolbar(const DrivesToolbar &) = delete;
	DrivesToolbar(DrivesToolbar &&) = delete;
	DrivesToolbar &operator=(const DrivesToolbar &) = delete;
	DrivesToolbar &operator=(DrivesToolbar &&) = delete;

	DrivesToolbarView *GetView() const;

private:
	static const int OPEN_IN_NEW_TAB_MENU_ITEM_ID = ShellContextMenu::MAX_SHELL_MENU_ID + 1;

	DrivesToolbar(DrivesToolbarView *view, std::unique_ptr<DriveModel> driveModel,
		BrowserWindow *browserWindow, const ResourceLoader *resourceLoader);
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
	void UpdateMenuEntries(HMENU menu, PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PidlChild> &pidlItems, IContextMenu *contextMenu) override;
	std::wstring GetHelpTextForItem(UINT menuItemId) override;
	bool HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PidlChild> &pidlItems,
		const std::wstring &verb) override;
	void HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PidlChild> &pidlItems,
		UINT menuItemId) override;

	void OnWindowDestroyed();

	DrivesToolbarView *const m_view;
	const std::unique_ptr<DriveModel> m_driveModel;
	BrowserWindow *const m_browserWindow;
	const ResourceLoader *const m_resourceLoader;
};
