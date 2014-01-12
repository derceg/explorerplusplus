#pragma once

#include <list>
#include <unordered_map>
#include <boost\serialization\strong_typedef.hpp>
#include "HardwareChangeNotifier.h"
#include "../Helper/FileContextMenuManager.h"

class CDrivesToolbar : public IFileContextMenuExternal, public NHardwareChangeNotifier::INotification
{
	friend LRESULT CALLBACK DrivesToolbarProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);
	friend LRESULT CALLBACK DrivesToolbarParentProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

public:

	/* This lifetime of this class is
	tied to its window. When the window
	is destroyed, this class will
	automatically free itself. */
	static CDrivesToolbar *Create(HWND hParent, UINT uIDStart, UINT uIDEnd, HINSTANCE hInstance, IExplorerplusplus *pexpp);

	HWND	GetHWND() const;

	/* IFileContextMenuExternal methods. */
	void	AddMenuEntries(LPITEMIDLIST pidlParent,const std::list<LPITEMIDLIST> &pidlItemList,DWORD_PTR dwData,HMENU hMenu);
	BOOL	HandleShellMenuItem(LPITEMIDLIST pidlParent,const std::list<LPITEMIDLIST> &pidlItemList,DWORD_PTR dwData,TCHAR *szCmd);
	void	HandleCustomMenuItem(LPITEMIDLIST pidlParent,const std::list<LPITEMIDLIST> &pidlItemList,int iCmd);

private:

	BOOST_STRONG_TYPEDEF(UINT,IDCounter);

	struct DriveInformation_t
	{
		int			Position;
		IDCounter	ID;
	};

	static const UINT_PTR SUBCLASS_ID = 0;
	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	static const int MIN_SHELL_MENU_ID = 1;
	static const int MAX_SHELL_MENU_ID = 1000;

	static const int MENU_ID_OPEN_IN_NEW_TAB = (MAX_SHELL_MENU_ID + 1);

	LRESULT CALLBACK DrivesToolbarProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	LRESULT CALLBACK DrivesToolbarParentProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

	CDrivesToolbar(HWND hParent, UINT uIDStart, UINT uIDEnd, HINSTANCE hInstance, IExplorerplusplus *pexpp);
	~CDrivesToolbar();

	void		InitializeToolbar(HWND hParent);

	void		InsertDrives();
	void		InsertDrive(const std::wstring &DrivePath);
	void		RemoveDrive(const std::wstring &DrivePath);

	int			GetSortedPosition(const std::wstring &DrivePath);
	DriveInformation_t GetDrivePosition(const std::wstring &DrivePath);
	std::wstring GetDrivePath(int iIndex);

	void		UpdateDriveIcon(const std::wstring &DrivePath);

	void		OnDeviceArrival(DEV_BROADCAST_HDR *dbh);
	void		OnDeviceRemoveComplete(DEV_BROADCAST_HDR *dbh);

	HWND		m_hToolbar;

	HINSTANCE	m_hInstance;

	UINT		m_uIDStart;
	UINT		m_uIDEnd;

	IExplorerplusplus *m_pexpp;

	std::unordered_map<IDCounter,std::wstring> m_mapID;
	IDCounter	m_IDCounter;
};