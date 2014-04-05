/******************************************************************
 *
 * Project: Explorer++
 * File: iShellView2.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Used to support new item creation in Windows XP. That is,
 * when an item is created using the new menu, this class
 * will select the file and place it in edit mode.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "iShellView2.h"


CShellView2::CShellView2(IExplorerplusplus *pexpp) :
m_pexpp(pexpp),
m_RefCount(1)
{

}

CShellView2::~CShellView2()
{

}

HRESULT __stdcall CShellView2::QueryInterface(REFIID iid,void **ppvObject)
{
	*ppvObject = NULL;

	if(iid == IID_IUnknown)
	{
		*ppvObject = static_cast<IUnknown *>(this);
	}
	else if(iid == IID_IOleWindow)
	{
		*ppvObject = static_cast<IOleWindow *>(this);
	}
	else if(iid == IID_IShellView)
	{
		*ppvObject = static_cast<IShellView *>(this);
	}
	else if(iid == IID_IShellView2)
	{
		*ppvObject = static_cast<IShellView2 *>(this);
	}

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall CShellView2::AddRef(void)
{
	return ++m_RefCount;
}

ULONG __stdcall CShellView2::Release(void)
{
	m_RefCount--;
	
	if(m_RefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_RefCount;
}

HRESULT CShellView2::CreateViewWindow2(LPSV2CVW2_PARAMS lpParams)
{
	UNREFERENCED_PARAMETER(lpParams);

	return S_OK;
}

HRESULT CShellView2::GetView(SHELLVIEWID *pvid,ULONG uView)
{
	UNREFERENCED_PARAMETER(pvid);
	UNREFERENCED_PARAMETER(uView);

	return S_OK;
}

HRESULT CShellView2::HandleRename(LPCITEMIDLIST pidlNew)
{
	UNREFERENCED_PARAMETER(pidlNew);

	return S_OK;
}

HRESULT CShellView2::SelectAndPositionItem(LPCITEMIDLIST pidlItem,UINT uFlags,POINT *ppt)
{
	UNREFERENCED_PARAMETER(uFlags);
	UNREFERENCED_PARAMETER(ppt);

	LPITEMIDLIST pidlComplete = NULL;
	LPITEMIDLIST pidlDirectory = NULL;

	/* The idlist passed is only a relative (child) one. Combine
	it with the tabs' current directory to get a full idlist. */
	pidlDirectory = m_pexpp->GetActiveShellBrowser()->QueryCurrentDirectoryIdl();
	pidlComplete = ILCombine(pidlDirectory,pidlItem);

	m_pexpp->GetActiveShellBrowser()->QueueRename(pidlComplete);

	CoTaskMemFree(pidlDirectory);
	CoTaskMemFree(pidlComplete);

	return S_OK;
}

HRESULT CShellView2::GetWindow(HWND *phwnd)
{
	UNREFERENCED_PARAMETER(phwnd);

	return S_OK;
}

HRESULT CShellView2::ContextSensitiveHelp(BOOL bHelp)
{
	UNREFERENCED_PARAMETER(bHelp);

	return S_OK;
}

HRESULT CShellView2::TranslateAccelerator(MSG *msg)
{
	UNREFERENCED_PARAMETER(msg);

	return S_OK;
}

HRESULT CShellView2::EnableModeless(BOOL fEnable)
{
	UNREFERENCED_PARAMETER(fEnable);

	return S_OK;
}

HRESULT CShellView2::UIActivate(UINT uActivate)
{
	UNREFERENCED_PARAMETER(uActivate);

	return S_OK;
}

HRESULT CShellView2::Refresh(void)
{
	return S_OK;
}

HRESULT CShellView2::CreateViewWindow(IShellView *psvPrevious,LPCFOLDERSETTINGS pfs,IShellBrowser *psb,RECT *prcView,HWND *phWnd)
{
	UNREFERENCED_PARAMETER(psvPrevious);
	UNREFERENCED_PARAMETER(pfs);
	UNREFERENCED_PARAMETER(psb);
	UNREFERENCED_PARAMETER(prcView);
	UNREFERENCED_PARAMETER(phWnd);

	return S_OK;
}

HRESULT CShellView2::DestroyViewWindow(void)
{
	return S_OK;
}

HRESULT CShellView2::GetCurrentInfo(LPFOLDERSETTINGS pfs)
{
	UNREFERENCED_PARAMETER(pfs);

	return S_OK;
}

HRESULT CShellView2::AddPropertySheetPages(DWORD dwReserved,LPFNSVADDPROPSHEETPAGE pfn,LPARAM lparam)
{
	UNREFERENCED_PARAMETER(dwReserved);
	UNREFERENCED_PARAMETER(pfn);
	UNREFERENCED_PARAMETER(lparam);

	return S_OK;
}

HRESULT CShellView2::SaveViewState(void)
{
	return S_OK;
}

HRESULT CShellView2::SelectItem(LPCITEMIDLIST pidlItem,SVSIF uFlags)
{
	UNREFERENCED_PARAMETER(pidlItem);
	UNREFERENCED_PARAMETER(uFlags);

	return S_OK;
}

HRESULT CShellView2::GetItemObject(UINT uItem,REFIID riid,void **ppv)
{
	UNREFERENCED_PARAMETER(uItem);
	UNREFERENCED_PARAMETER(riid);
	UNREFERENCED_PARAMETER(ppv);

	return S_OK;
}