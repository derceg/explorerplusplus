// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "NewMenuClient.h"
#include "CoreInterface.h"
#include "ShellBrowser/ShellBrowser.h"
#include "../Helper/Logging.h"

wil::com_ptr_nothrow<NewMenuClient> NewMenuClient::Create(IExplorerplusplus *expp)
{
	wil::com_ptr_nothrow<NewMenuClient> newMenuClient;
	newMenuClient.attach(new NewMenuClient(expp));
	return newMenuClient;
}

NewMenuClient::NewMenuClient(IExplorerplusplus *pexpp) : m_refCount(1), m_pexpp(pexpp)
{
}

// INewMenuClient
IFACEMETHODIMP NewMenuClient::IncludeItems(NMCII_FLAGS *flags)
{
	/* flags will be one of:
	NMCII_ITEMS
	NMCII_FOLDERS
	Which one of these is selected determines which items are shown on the 'new' menu.
	If NMCII_ITEMS is selected, only files will be shown (meaning 'New Folder' will NOT appear).
	If NMCII_FOLDERS is selected, only folders will be shown (this means that in most cases, only
	'New Folder' and 'New Briefcase' will be shown).
	Therefore, to get all the items, OR the two flags together to show files and folders. */

	*flags = NMCII_ITEMS | NMCII_FOLDERS;

	return S_OK;
}

IFACEMETHODIMP NewMenuClient::SelectAndEditItem(PCIDLIST_ABSOLUTE pidlItem, NMCSAEI_FLAGS flags)
{
	switch (flags)
	{
	// This would usually cause the item to be selected first, then renamed. In this case however,
	// the item is selected and renamed in one operation, so this state can be ignored.
	case NMCSAEI_SELECT:
		break;

	// Now, start an in-place rename of the item.
	case NMCSAEI_EDIT:
	case NMCSAEI_EDIT_WINDOWS_10:
		LOG(info) << _T("Starting in-place rename of item created via new menu");
		m_pexpp->GetActiveShellBrowser()->QueueRename(pidlItem);
		break;
	}

	return S_OK;
}

// IUnknown
IFACEMETHODIMP NewMenuClient::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = nullptr;

	if (iid == IID_IUnknown)
	{
		*ppvObject = static_cast<IUnknown *>(this);
	}
	else if (iid == IID_INewMenuClient)
	{
		*ppvObject = static_cast<INewMenuClient *>(this);
	}

	if (*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

IFACEMETHODIMP_(ULONG) NewMenuClient::AddRef()
{
	return ++m_refCount;
}

IFACEMETHODIMP_(ULONG) NewMenuClient::Release()
{
	m_refCount--;

	if (m_refCount == 0)
	{
		delete this;
		return 0;
	}

	return m_refCount;
}