// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DriveModel.h"

class DrivesToolbarView;
__interface IExplorerplusplus;
struct MouseEvent;
class Navigation;

class DrivesToolbar
{
public:
	static DrivesToolbar *Create(HWND parent, IExplorerplusplus *coreInterface, HINSTANCE instance,
		Navigation *navigation);

	DrivesToolbar(const DrivesToolbar &) = delete;
	DrivesToolbar(DrivesToolbar &&) = delete;
	DrivesToolbar &operator=(const DrivesToolbar &) = delete;
	DrivesToolbar &operator=(DrivesToolbar &&) = delete;

	DrivesToolbarView *GetView() const;

private:
	DrivesToolbar(HWND parent, IExplorerplusplus *coreInterface, HINSTANCE instance,
		Navigation *navigation);
	~DrivesToolbar() = default;

	void Initialize();

	void AddDrives();
	void AddDriveAtIndex(const std::wstring &drivePath, size_t index);

	void OnDriveAdded(const std::wstring &path, size_t index);
	void OnDriveUpdated(const std::wstring &path);
	void OnDriveRemoved(const std::wstring &path, size_t oldIndex);

	void OnButtonClicked(const std::wstring &drivePath, const MouseEvent &event);
	void OnButtonMiddleClicked(const std::wstring &drivePath, const MouseEvent &event);
	void OnButtonRightClicked(const std::wstring &drivePath, const MouseEvent &event);

	void OnWindowDestroyed();

	DrivesToolbarView *m_view;
	IExplorerplusplus *m_coreInterface;
	Navigation *m_navigation;
	DriveModel m_driveModel;
};
