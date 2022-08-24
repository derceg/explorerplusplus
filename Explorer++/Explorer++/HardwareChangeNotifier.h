// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <list>

class HardwareChangeObserver
{
public:
	virtual ~HardwareChangeObserver() = default;

	virtual void OnDeviceArrival(DEV_BROADCAST_HDR *dbh) = 0;
	virtual void OnDeviceRemoveComplete(DEV_BROADCAST_HDR *dbh) = 0;
};

class HardwareChangeNotifier
{
public:
	static HardwareChangeNotifier &GetInstance();

	void AddObserver(HardwareChangeObserver *observer);
	void RemoveObserver(HardwareChangeObserver *observer);

	void NotifyDeviceArrival(DEV_BROADCAST_HDR *dbh);
	void NotifyDeviceRemovalComplete(DEV_BROADCAST_HDR *dbh);

private:
	enum class NotificationType
	{
		Arrival,
		RemovalComplete
	};

	HardwareChangeNotifier() = default;

	HardwareChangeNotifier(const HardwareChangeNotifier &) = delete;
	HardwareChangeNotifier &operator=(const HardwareChangeNotifier &) = delete;

	void NotifyObservers(NotificationType notificationType, DEV_BROADCAST_HDR *dbh);

	std::list<HardwareChangeObserver *> m_Observers;
};
