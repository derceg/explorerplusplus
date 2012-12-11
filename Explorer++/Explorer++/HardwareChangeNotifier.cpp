/******************************************************************
 *
 * Project: Explorer++
 * File: HardwareChangeNotifier.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Sends out hardware change notifications.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "HardwareChangeNotifier.h"


CHardwareChangeNotifier::CHardwareChangeNotifier()
{

}

CHardwareChangeNotifier& CHardwareChangeNotifier::GetInstance()
{
	static CHardwareChangeNotifier hcn;
	return hcn;
}

void CHardwareChangeNotifier::AddObserver(NHardwareChangeNotifier::INotification *hcn)
{
	m_Observers.push_back(hcn);
}

void CHardwareChangeNotifier::RemoveObserver(NHardwareChangeNotifier::INotification *hcn)
{
	auto itr = std::find_if(m_Observers.begin(),m_Observers.end(),
		[hcn](const NHardwareChangeNotifier::INotification *hcnCurrent){return hcnCurrent == hcn;});

	if(itr != m_Observers.end())
	{
		m_Observers.erase(itr);
	}
}

void CHardwareChangeNotifier::NotifyDeviceArrival(DEV_BROADCAST_HDR *dbh)
{
	NotifyObservers(NOTIFY_DEVICE_ARRIVAL,dbh);
}

void CHardwareChangeNotifier::NotifyDeviceRemovalComplete(DEV_BROADCAST_HDR *dbh)
{
	NotifyObservers(NOTIFY_DEVICE_REMOVAL_COMPLETE,dbh);
}

void CHardwareChangeNotifier::NotifyObservers(NotificationType_t NotificationType,DEV_BROADCAST_HDR *dbh)
{
	for each(auto hcn in m_Observers)
	{
		switch(NotificationType)
		{
		case NOTIFY_DEVICE_ARRIVAL:
			hcn->OnDeviceArrival(dbh);
			break;

		case NOTIFY_DEVICE_REMOVAL_COMPLETE:
			hcn->OnDeviceRemoveComplete(dbh);
			break;
		}
	}
}