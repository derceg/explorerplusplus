// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/com.h>
#include <ShObjIdl.h>

__interface IExplorerplusplus;

class NewMenuClient : public INewMenuClient
{
public:
	static wil::com_ptr_nothrow<NewMenuClient> Create(IExplorerplusplus *expp);

	// INewMenuClient
	IFACEMETHODIMP IncludeItems(NMCII_FLAGS *flags);
	IFACEMETHODIMP SelectAndEditItem(PCIDLIST_ABSOLUTE pidlItem, NMCSAEI_FLAGS flags);

	// IUnknown
	STDMETHODIMP QueryInterface(REFIID iid, void **ppvObject);
	IFACEMETHODIMP_(ULONG) AddRef();
	IFACEMETHODIMP_(ULONG) Release();

private:
	NewMenuClient(IExplorerplusplus *pexpp);

	// From testing, this value is sent on Windows 10 when an item needs to be put into rename mode.
	static const int NMCSAEI_EDIT_WINDOWS_10 = 0x0003;

	ULONG m_refCount;
	IExplorerplusplus *m_pexpp;
};