// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DriveWatcherImpl.h"
#include "../Helper/DriveInfo.h"
#include <format>

DriveWatcherImpl::DriveWatcherImpl(HWND topLevelWindow)
{
	// WM_DEVICECHANGE is only sent to top-level windows.
	auto style = GetWindowLongPtr(topLevelWindow, GWL_STYLE);
	DCHECK(style && WI_IsFlagClear(style, WS_CHILD));

	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(topLevelWindow,
		std::bind_front(&DriveWatcherImpl::WndProc, this)));
}

LRESULT DriveWatcherImpl::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DEVICECHANGE:
		OnDeviceChange(wParam, lParam);
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void DriveWatcherImpl::OnDeviceChange(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case DBT_DEVICEARRIVAL:
	case DBT_DEVICEREMOVECOMPLETE:
		OnDeviceArrivedOrRemoved(wParam == DBT_DEVICEARRIVAL ? DeviceChangeType::Arrival
															 : DeviceChangeType::Removal,
			reinterpret_cast<DEV_BROADCAST_HDR *>(lParam));
		break;
	}
}

void DriveWatcherImpl::OnDeviceArrivedOrRemoved(DeviceChangeType deviceChangeType,
	const DEV_BROADCAST_HDR *deviceBroadcast)
{
	if (deviceBroadcast->dbch_devicetype != DBT_DEVTYP_VOLUME)
	{
		return;
	}

	auto *volume = reinterpret_cast<const DEV_BROADCAST_VOLUME *>(deviceBroadcast);

	TCHAR driveLetter = GetDriveLetterFromMask(volume->dbcv_unitmask);
	std::wstring path = std::format(L"{}:\\", driveLetter);

	if (WI_IsFlagSet(volume->dbcv_flags, DBTF_MEDIA))
	{
		m_driveUpdatedSignal(path);
	}
	else
	{
		if (deviceChangeType == DeviceChangeType::Arrival)
		{
			m_driveAddedSignal(path);
		}
		else
		{
			m_driveRemovedSignal(path);
		}
	}
}

// DriveWatcher
boost::signals2::connection DriveWatcherImpl::AddDriveAddedObserver(
	const DriveAddedSignal::slot_type &observer)
{
	return m_driveAddedSignal.connect(observer);
}

boost::signals2::connection DriveWatcherImpl::AddDriveUpdatedObserver(
	const DriveUpdatedSignal::slot_type &observer)
{
	return m_driveUpdatedSignal.connect(observer);
}

boost::signals2::connection DriveWatcherImpl::AddDriveRemovedObserver(
	const DriveRemovedSignal::slot_type &observer)
{
	return m_driveRemovedSignal.connect(observer);
}
