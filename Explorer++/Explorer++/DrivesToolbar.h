// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "HardwareChangeNotifier.h"
#include "Navigation.h"
#include "../Helper/BaseWindow.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <boost/serialization/strong_typedef.hpp>
#include <unordered_map>

__interface IExplorerplusplus;

class DrivesToolbar :
	public BaseWindow,
	public IFileContextMenuExternal,
	public NHardwareChangeNotifier::INotification
{
public:
	static DrivesToolbar *Create(HWND hParent, UINT uIDStart, UINT uIDEnd, HINSTANCE hInstance,
		IExplorerplusplus *pexpp, Navigation *navigation);

	/* IFileContextMenuExternal methods. */
	void UpdateMenuEntries(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, IContextMenu *contextMenu,
		HMENU hMenu) override;
	BOOL HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, const TCHAR *szCmd) override;
	void HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PITEMID_CHILD> &pidlItems, int iCmd) override;

protected:
	INT_PTR OnMButtonUp(const POINTS *pts, UINT keysDown) override;

private:
	BOOST_STRONG_TYPEDEF(UINT, IDCounter);

	struct DriveInformation
	{
		int Position;
		IDCounter ID;
	};

	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	static const int MIN_SHELL_MENU_ID = 1;
	static const int MAX_SHELL_MENU_ID = 1000;

	static const int MENU_ID_OPEN_IN_NEW_TAB = (MAX_SHELL_MENU_ID + 1);

	static LRESULT CALLBACK DrivesToolbarParentProcStub(HWND hwnd, UINT uMsg, WPARAM wParam,
		LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK DrivesToolbarParentProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	DrivesToolbar(HWND hParent, UINT uIDStart, UINT uIDEnd, HINSTANCE hInstance,
		IExplorerplusplus *pexpp, Navigation *navigation);
	~DrivesToolbar();

	static HWND CreateDrivesToolbar(HWND hParent);

	void Initialize(HWND hParent);

	void InsertDrives();
	void InsertDrive(const std::wstring &DrivePath);
	void RemoveDrive(const std::wstring &DrivePath);

	int GetSortedPosition(const std::wstring &DrivePath);
	DriveInformation GetDrivePosition(const std::wstring &DrivePath);
	std::wstring GetDrivePath(int iIndex);

	void UpdateDriveIcon(const std::wstring &DrivePath);

	void OnDeviceArrival(DEV_BROADCAST_HDR *dbh) override;
	void OnDeviceRemoveComplete(DEV_BROADCAST_HDR *dbh) override;

	HINSTANCE m_hInstance;

	UINT m_uIDStart;
	UINT m_uIDEnd;

	IExplorerplusplus *m_pexpp;
	Navigation *m_navigation;

	struct IDCounterHasher
	{
		size_t operator()(const IDCounter &t) const
		{
			return (size_t) t;
		}
	};

	std::unordered_map<IDCounter, std::wstring, IDCounterHasher> m_mapID;

	IDCounter m_IDCounter;

	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;
};