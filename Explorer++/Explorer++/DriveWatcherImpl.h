// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DriveWatcher.h"
#include "../Helper/WindowSubclass.h"
#include <Dbt.h>
#include <memory>
#include <vector>

class DriveWatcherImpl : public DriveWatcher
{
public:
	DriveWatcherImpl(HWND topLevelWindow);

	DriveWatcherImpl(const DriveWatcherImpl &) = delete;
	DriveWatcherImpl(DriveWatcherImpl &&) = delete;
	DriveWatcherImpl &operator=(const DriveWatcherImpl &) = delete;
	DriveWatcherImpl &operator=(DriveWatcherImpl &&) = delete;

	// DriveWatcher
	boost::signals2::connection AddDriveAddedObserver(
		const DriveAddedSignal::slot_type &observer) override;
	boost::signals2::connection AddDriveUpdatedObserver(
		const DriveUpdatedSignal::slot_type &observer) override;
	boost::signals2::connection AddDriveRemovedObserver(
		const DriveRemovedSignal::slot_type &observer) override;

private:
	enum class DeviceChangeType
	{
		Arrival,
		Removal
	};

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnDeviceChange(WPARAM wParam, LPARAM lParam);
	void OnDeviceArrivedOrRemoved(DeviceChangeType deviceChangeType,
		const DEV_BROADCAST_HDR *deviceBroadcast);

	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;

	DriveAddedSignal m_driveAddedSignal;
	DriveUpdatedSignal m_driveUpdatedSignal;
	DriveRemovedSignal m_driveRemovedSignal;
};
