// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <list>

namespace NHardwareChangeNotifier
{
	__interface INotification
	{
		void	OnDeviceArrival(DEV_BROADCAST_HDR *dbh);
		void	OnDeviceRemoveComplete(DEV_BROADCAST_HDR *dbh);
	};
}

class HardwareChangeNotifier
{
public:

	static HardwareChangeNotifier &GetInstance();

	void	AddObserver(NHardwareChangeNotifier::INotification *hcn);
	void	RemoveObserver(NHardwareChangeNotifier::INotification *hcn);

	void	NotifyDeviceArrival(DEV_BROADCAST_HDR *dbh);
	void	NotifyDeviceRemovalComplete(DEV_BROADCAST_HDR *dbh);

private:

	enum NotificationType_t
	{
		NOTIFY_DEVICE_ARRIVAL,
		NOTIFY_DEVICE_REMOVAL_COMPLETE
	};

	HardwareChangeNotifier();

	HardwareChangeNotifier(const HardwareChangeNotifier &);
	HardwareChangeNotifier & operator=(const HardwareChangeNotifier &);

	void	NotifyObservers(NotificationType_t NotificationType,DEV_BROADCAST_HDR *dbh);

	std::list<NHardwareChangeNotifier::INotification *>	m_Observers;
};