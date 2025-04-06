// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DriveWatcher.h"
#include <Dbt.h>
#include <vector>

class EventWindow;

class DriveWatcherImpl : public DriveWatcher
{
public:
	DriveWatcherImpl(EventWindow *eventWindow);

	DriveWatcherImpl(const DriveWatcherImpl &) = delete;
	DriveWatcherImpl(DriveWatcherImpl &&) = delete;
	DriveWatcherImpl &operator=(const DriveWatcherImpl &) = delete;
	DriveWatcherImpl &operator=(DriveWatcherImpl &&) = delete;

private:
	enum class DeviceChangeType
	{
		Arrival,
		Removal
	};

	void OnEventWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnDeviceChange(WPARAM wParam, LPARAM lParam);
	void OnDeviceArrivedOrRemoved(DeviceChangeType deviceChangeType,
		const DEV_BROADCAST_HDR *deviceBroadcast);

	std::vector<boost::signals2::scoped_connection> m_connections;
};
