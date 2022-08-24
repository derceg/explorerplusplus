// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DriveModel.h"
#include "../Helper/DriveInfo.h"
#include <format>
#include <memory>

DriveModel::DriveModel()
{
	InitializeDriveList();

	HardwareChangeNotifier::GetInstance().AddObserver(this);
}

DriveModel::~DriveModel()
{
	HardwareChangeNotifier::GetInstance().RemoveObserver(this);
}

void DriveModel::InitializeDriveList()
{
	DWORD size = GetLogicalDriveStrings(0, nullptr);

	if (size == 0)
	{
		return;
	}

	auto driveStrings = std::make_unique<TCHAR[]>(size);
	size = GetLogicalDriveStrings(size, driveStrings.get());

	if (size == 0)
	{
		return;
	}

	TCHAR *currentDrive = driveStrings.get();

	while (*currentDrive != '\0')
	{
		m_drives.emplace(currentDrive);

		currentDrive += (lstrlen(currentDrive) + 1);
	}
}

const std::set<std::wstring> &DriveModel::GetDrives() const
{
	return m_drives;
}

std::optional<size_t> DriveModel::GetDriveIndex(const std::wstring &path) const
{
	auto itr = m_drives.find(path);

	if (itr == m_drives.end())
	{
		return std::nullopt;
	}

	return std::distance(m_drives.begin(), itr);
}

boost::signals2::connection DriveModel::AddDriveAddedObserver(
	const DriveAddedSignal::slot_type &observer)
{
	return m_driveAddedSignal.connect(observer);
}

boost::signals2::connection DriveModel::AddDriveUpdatedObserver(
	const DriveUpdatedSignal::slot_type &observer)
{
	return m_driveUpdatedSignal.connect(observer);
}

boost::signals2::connection DriveModel::AddDriveRemovedObserver(
	const DriveRemovedSignal::slot_type &observer)
{
	return m_driveRemovedSignal.connect(observer);
}

void DriveModel::OnDeviceArrival(DEV_BROADCAST_HDR *deviceBroadcast)
{
	if (deviceBroadcast->dbch_devicetype != DBT_DEVTYP_VOLUME)
	{
		return;
	}

	auto *volume = reinterpret_cast<DEV_BROADCAST_VOLUME *>(deviceBroadcast);

	TCHAR driveLetter = GetDriveLetterFromMask(volume->dbcv_unitmask);
	std::wstring path = std::format(L"{}:\\", driveLetter);

	if (WI_IsFlagSet(volume->dbcv_flags, DBTF_MEDIA))
	{
		m_driveUpdatedSignal(path.c_str());
	}
	else
	{
		AddDrive(path.c_str());
	}
}

void DriveModel::OnDeviceRemoveComplete(DEV_BROADCAST_HDR *deviceBroadcast)
{
	if (deviceBroadcast->dbch_devicetype != DBT_DEVTYP_VOLUME)
	{
		return;
	}

	auto *volume = reinterpret_cast<DEV_BROADCAST_VOLUME *>(deviceBroadcast);

	TCHAR driveLetter = GetDriveLetterFromMask(volume->dbcv_unitmask);
	std::wstring path = std::format(L"{}:\\", driveLetter);

	if (WI_IsFlagSet(volume->dbcv_flags, DBTF_MEDIA))
	{
		m_driveUpdatedSignal(path.c_str());
	}
	else
	{
		RemoveDrive(path.c_str());
	}
}

void DriveModel::AddDrive(const std::wstring &path)
{
	if (m_drives.contains(path))
	{
		return;
	}

	auto [itr, inserted] = m_drives.emplace(path);

	size_t index = std::distance(m_drives.begin(), itr);

	m_driveAddedSignal(path, index);
}

void DriveModel::RemoveDrive(const std::wstring &path)
{
	auto itr = m_drives.find(path);

	if (itr == m_drives.end())
	{
		return;
	}

	size_t index = std::distance(m_drives.begin(), itr);

	m_drives.erase(itr);

	m_driveRemovedSignal(path, index);
}
