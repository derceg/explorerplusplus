// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <list>

namespace NHardwareChangeNotifier
{
	__interface INotification
	{
		void OnDeviceArrival(DEV_BROADCAST_HDR * dbh);
		void OnDeviceRemoveComplete(DEV_BROADCAST_HDR * dbh);
	};
}

class HardwareChangeNotifier
{
public:
	static HardwareChangeNotifier &GetInstance();

	void AddObserver(NHardwareChangeNotifier::INotification *hcn);
	void RemoveObserver(NHardwareChangeNotifier::INotification *hcn);

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

	std::list<NHardwareChangeNotifier::INotification *> m_Observers;
};