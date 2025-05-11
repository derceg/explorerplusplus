// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>
#include <string>
#include <vector>

class BrowserWindow;
class DriveModel;
class DrivesToolbarView;
struct MouseEvent;
class ResourceLoader;

class DrivesToolbar
{
public:
	static DrivesToolbar *Create(DrivesToolbarView *view, DriveModel *driveModel,
		BrowserWindow *browser, const ResourceLoader *resourceLoader);

	DrivesToolbar(const DrivesToolbar &) = delete;
	DrivesToolbar(DrivesToolbar &&) = delete;
	DrivesToolbar &operator=(const DrivesToolbar &) = delete;
	DrivesToolbar &operator=(DrivesToolbar &&) = delete;

	DrivesToolbarView *GetView() const;

private:
	DrivesToolbar(DrivesToolbarView *view, DriveModel *driveModel, BrowserWindow *browser,
		const ResourceLoader *resourceLoader);
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

	void OnWindowDestroyed();

	DrivesToolbarView *const m_view;
	DriveModel *const m_driveModel;
	BrowserWindow *const m_browser;
	const ResourceLoader *const m_resourceLoader;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
