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
	return S_OK;
}

HRESULT CShellView2::GetView(SHELLVIEWID *pvid,ULONG uView)
{
	return S_OK;
}

HRESULT CShellView2::HandleRename(LPCITEMIDLIST pidlNew)
{
	return S_OK;
}

HRESULT CShellView2::SelectAndPositionItem(LPCITEMIDLIST pidlItem,UINT uFlags,POINT *ppt)
{
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

HRESULT CShellView2::GetWindow(HWND *)
{
	return S_OK;
}

HRESULT CShellView2::ContextSensitiveHelp(BOOL bHelp)
{
	return S_OK;
}

HRESULT CShellView2::TranslateAccelerator(MSG *msg)
{
	return S_OK;
}

HRESULT CShellView2::EnableModeless(BOOL fEnable)
{
	return S_OK;
}

HRESULT CShellView2::UIActivate(UINT uActivate)
{
	return S_OK;
}

HRESULT CShellView2::Refresh(void)
{
	return S_OK;
}

HRESULT CShellView2::CreateViewWindow(IShellView *psvPrevious,LPCFOLDERSETTINGS pfs,IShellBrowser *psb,RECT *prcView,HWND *phWnd)
{
	return S_OK;
}

HRESULT CShellView2::DestroyViewWindow(void)
{
	return S_OK;
}

HRESULT CShellView2::GetCurrentInfo(LPFOLDERSETTINGS pfs)
{
	return S_OK;
}

HRESULT CShellView2::AddPropertySheetPages(DWORD dwReserved,LPFNSVADDPROPSHEETPAGE pfn,LPARAM lparam)
{
	return S_OK;
}

HRESULT CShellView2::SaveViewState(void)
{
	return S_OK;
}

HRESULT CShellView2::SelectItem(LPCITEMIDLIST pidlItem,SVSIF uFlags)
{
	return S_OK;
}

HRESULT CShellView2::GetItemObject(UINT uItem,REFIID riid,void **ppv)
{
	return S_OK;
}