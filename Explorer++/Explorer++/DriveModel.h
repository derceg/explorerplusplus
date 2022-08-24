// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "HardwareChangeNotifier.h"
#include <boost/signals2.hpp>
#include <Dbt.h>
#include <optional>
#include <set>

// Maintains a sorted list of drives and provides notifications on drive additions/removals.
class DriveModel : private HardwareChangeObserver
{
public:
	using DriveAddedSignal = boost::signals2::signal<void(const std::wstring &path, size_t index)>;
	using DriveUpdatedSignal = boost::signals2::signal<void(const std::wstring &path)>;
	using DriveRemovedSignal =
		boost::signals2::signal<void(const std::wstring &path, size_t oldIndex)>;

	DriveModel();
	~DriveModel();

	DriveModel(const DriveModel &) = delete;
	DriveModel(DriveModel &&) = delete;
	DriveModel &operator=(const DriveModel &) = delete;
	DriveModel &operator=(DriveModel &&) = delete;

	const std::set<std::wstring> &GetDrives() const;
	std::optional<size_t> GetDriveIndex(const std::wstring &path) const;

	boost::signals2::connection AddDriveAddedObserver(const DriveAddedSignal::slot_type &observer);
	boost::signals2::connection AddDriveUpdatedObserver(
		const DriveUpdatedSignal::slot_type &observer);
	boost::signals2::connection AddDriveRemovedObserver(
		const DriveRemovedSignal::slot_type &observer);

private:
	void InitializeDriveList();

	// HardwareChangeObserver
	void OnDeviceArrival(DEV_BROADCAST_HDR *deviceBroadcast) override;
	void OnDeviceRemoveComplete(DEV_BROADCAST_HDR *deviceBroadcast) override;

	void AddDrive(const std::wstring &path);
	void RemoveDrive(const std::wstring &path);

	std::set<std::wstring> m_drives;

	DriveAddedSignal m_driveAddedSignal;
	DriveUpdatedSignal m_driveUpdatedSignal;
	DriveRemovedSignal m_driveRemovedSignal;
};
