#pragma once

#include <list>
#include <unordered_map>
#include <boost\serialization\strong_typedef.hpp>
#include "HardwareChangeNotifier.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/BaseWindow.h"

class CDrivesToolbar : public CBaseWindow, public IFileContextMenuExternal, public NHardwareChangeNotifier::INotification
{
	friend LRESULT CALLBACK DrivesToolbarParentProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

public:

	static CDrivesToolbar *Create(HWND hParent, UINT uIDStart, UINT uIDEnd, HINSTANCE hInstance, IExplorerplusplus *pexpp);

	/* IFileContextMenuExternal methods. */
	void	AddMenuEntries(LPCITEMIDLIST pidlParent,const std::list<LPITEMIDLIST> &pidlItemList,DWORD_PTR dwData,HMENU hMenu);
	BOOL	HandleShellMenuItem(LPCITEMIDLIST pidlParent,const std::list<LPITEMIDLIST> &pidlItemList,DWORD_PTR dwData,const TCHAR *szCmd);
	void	HandleCustomMenuItem(LPCITEMIDLIST pidlParent,const std::list<LPITEMIDLIST> &pidlItemList,int iCmd);

protected:

	INT_PTR	OnMButtonUp(const POINTS *pts);

private:

	BOOST_STRONG_TYPEDEF(UINT,IDCounter);

	struct DriveInformation_t
	{
		int			Position;
		IDCounter	ID;
	};

	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	static const int MIN_SHELL_MENU_ID = 1;
	static const int MAX_SHELL_MENU_ID = 1000;

	static const int MENU_ID_OPEN_IN_NEW_TAB = (MAX_SHELL_MENU_ID + 1);

	LRESULT CALLBACK DrivesToolbarParentProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

	CDrivesToolbar(HWND hParent, UINT uIDStart, UINT uIDEnd, HINSTANCE hInstance, IExplorerplusplus *pexpp);
	~CDrivesToolbar();

	static HWND	CreateDrivesToolbar(HWND hParent);

	void		Initialize(HWND hParent);

	void		InsertDrives();
	void		InsertDrive(const std::wstring &DrivePath);
	void		RemoveDrive(const std::wstring &DrivePath);

	int			GetSortedPosition(const std::wstring &DrivePath);
	DriveInformation_t GetDrivePosition(const std::wstring &DrivePath);
	std::wstring GetDrivePath(int iIndex);

	void		UpdateDriveIcon(const std::wstring &DrivePath);

	void		OnDeviceArrival(DEV_BROADCAST_HDR *dbh);
	void		OnDeviceRemoveComplete(DEV_BROADCAST_HDR *dbh);

	HINSTANCE	m_hInstance;

	UINT		m_uIDStart;
	UINT		m_uIDEnd;

	IExplorerplusplus *m_pexpp;

	std::unordered_map<IDCounter,std::wstring> m_mapID;
	IDCounter	m_IDCounter;
};