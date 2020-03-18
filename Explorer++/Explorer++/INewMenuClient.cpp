// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Used to support new item creation in Windows Vista and later.
 * That is, when an item is created using the new menu, this
 * class will select the file and place it in edit mode.
 */

#include "stdafx.h"
#include "INewMenuClient.h"
#include "CoreInterface.h"
#include "ShellBrowser/ShellBrowser.h"
#include "../Helper/Logging.h"

NewMenuClient::NewMenuClient(IExplorerplusplus *pexpp) :
m_pexpp(pexpp),
m_RefCount(1)
{

}

HRESULT __stdcall NewMenuClient::QueryInterface(REFIID iid,void **ppvObject)
{
	*ppvObject = nullptr;

	if(iid == IID_IUnknown)
	{
		*ppvObject = static_cast<IUnknown *>(this);
	}
	else if(iid == IID_INewMenuClient)
	{
		*ppvObject = static_cast<INewMenuClient *>(this);
	}

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall NewMenuClient::AddRef()
{
	return ++m_RefCount;
}

ULONG __stdcall NewMenuClient::Release()
{
	m_RefCount--;
	
	if(m_RefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_RefCount;
}

HRESULT NewMenuClient::IncludeItems(NMCII_FLAGS *pFlags)
{
	/* pFlags will be one of:
	NMCII_ITEMS
	NMCII_FOLDERS
	Which one of these is selected determines which
	items are shown on the 'new' menu.
	If NMCII_ITEMS is selected, only files will be
	shown (meaning 'New Folder' will NOT appear).
	If NMCII_FOLDERS is selected, only folders will
	be shown (this means that in most cases, only
	'New Folder' and 'New Briefcase' will be shown).
	Therefore, to get all the items, OR the two flags
	together to show files and folders. */

	*pFlags = NMCII_ITEMS|NMCII_FOLDERS;

	return S_OK;
}

HRESULT NewMenuClient::SelectAndEditItem(PCIDLIST_ABSOLUTE pidlItem,NMCSAEI_FLAGS flags)
{
	switch(flags)
	{
		/* This would usually cause the
		item to be selected first, then
		renamed. In this case however,
		the item is selected and renamed
		in one operation, so this state
		can be ignored. */
		case NMCSAEI_SELECT:
			break;

		/* Now, start an in-place rename
		of the item. */
		case NMCSAEI_EDIT:
		case NMCSAEI_EDIT_WINDOWS_10:
			LOG(info) << _T("Starting in-place rename of item created via new menu");
			m_pexpp->GetActiveShellBrowser()->QueueRename(pidlItem);
			break;
	}

	return S_OK;
}