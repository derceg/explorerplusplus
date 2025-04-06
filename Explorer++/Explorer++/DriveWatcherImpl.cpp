// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DriveWatcherImpl.h"
#include "EventWindow.h"
#include "../Helper/DriveInfo.h"
#include <format>

DriveWatcherImpl::DriveWatcherImpl(EventWindow *eventWindow)
{
	m_connections.push_back(eventWindow->windowMessageSignal.AddObserver(
		std::bind_front(&DriveWatcherImpl::OnEventWindowMessage, this)));
}

void DriveWatcherImpl::OnEventWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(hwnd);

	switch (msg)
	{
	case WM_DEVICECHANGE:
		OnDeviceChange(wParam, lParam);
		break;
	}
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
