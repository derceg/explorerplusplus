// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DriveModel.h"
#include "DriveEnumerator.h"
#include "DriveWatcher.h"

DriveModel::DriveModel(std::unique_ptr<DriveEnumerator> driveEnumerator, DriveWatcher *driveWatcher)
{
	auto drivesResult = driveEnumerator->GetDrives();

	// Retrieving the list of drives shouldn't fail, so any failure here would be unexpected. But
	// there's not much that can be done in that situation, so any error is ignored.
	if (drivesResult)
	{
		m_drives = drivesResult.value();
	}

	m_connections.push_back(
		driveWatcher->AddDriveAddedObserver(std::bind_front(&DriveModel::OnDriveAdded, this)));
	m_connections.push_back(
		driveWatcher->AddDriveUpdatedObserver(std::bind_front(&DriveModel::OnDriveUpdated, this)));
	m_connections.push_back(
		driveWatcher->AddDriveRemovedObserver(std::bind_front(&DriveModel::OnDriveRemoved, this)));
}

DriveModel::~DriveModel() = default;

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

void DriveModel::OnDriveAdded(const std::wstring &path)
{
	AddDrive(path);
}

void DriveModel::OnDriveUpdated(const std::wstring &path)
{
	if (!m_drives.contains(path))
	{
		return;
	}

	m_driveUpdatedSignal(path);
}

void DriveModel::OnDriveRemoved(const std::wstring &path)
{
	RemoveDrive(path);
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
