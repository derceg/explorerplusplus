// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"

class CNewMenuClient : public INewMenuClient
{
public:

	CNewMenuClient(IExplorerplusplus *pexpp);

private:

	/* These are used with the IncludeItems()
	method of INewMenuClient. INewMenuClient
	is used to support the shell 'new' menu. */
	static const int NMCII_ITEMS = 0x0001;
	static const int NMCII_FOLDERS = 0x0002;

	/* These two flags are used with the
	SelectAndEdit() method of INewClient. */
	static const int NMCSAEI_SELECT = 0x0000;
	static const int NMCSAEI_EDIT = 0x0001;

	/* From testing, this value is sent
	on Windows 10 when an item needs to
	be put into rename mode. */
	static const int NMCSAEI_EDIT_WINDOWS_10 = 0x0003;

	/* IUnknown methods. */
	HRESULT __stdcall	QueryInterface(REFIID iid,void **ppvObject);
	ULONG __stdcall		AddRef(void);
	ULONG __stdcall		Release(void);

	/* INewMenuClient methods. */
	HRESULT __stdcall	IncludeItems(NMCII_FLAGS *pFlags);
	HRESULT __stdcall	SelectAndEditItem(PCIDLIST_ABSOLUTE pidlItem,NMCSAEI_FLAGS flags);

	ULONG				m_RefCount;
	IExplorerplusplus	*m_pexpp;
};