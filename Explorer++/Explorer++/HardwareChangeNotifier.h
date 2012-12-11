#ifndef HARDWARECHANGENOTIFIER_INCLUDED
#define HARDWARECHANGENOTIFIER_INCLUDED

#include <list>

namespace NHardwareChangeNotifier
{
	__interface INotification
	{
		void	OnDeviceArrival(DEV_BROADCAST_HDR *dbh);
		void	OnDeviceRemoveComplete(DEV_BROADCAST_HDR *dbh);
	};
}

class CHardwareChangeNotifier
{
public:

	static CHardwareChangeNotifier &GetInstance();

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

	CHardwareChangeNotifier();

	CHardwareChangeNotifier(const CHardwareChangeNotifier &);
	CHardwareChangeNotifier & operator=(const CHardwareChangeNotifier &);

	void	NotifyObservers(NotificationType_t NotificationType,DEV_BROADCAST_HDR *dbh);

	std::list<NHardwareChangeNotifier::INotification *>	m_Observers;
};

#endif