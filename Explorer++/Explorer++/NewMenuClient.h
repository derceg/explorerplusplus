// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <winrt/base.h>
#include <ShObjIdl.h>

__interface IExplorerplusplus;

class NewMenuClient : public winrt::implements<NewMenuClient, INewMenuClient>
{
public:
	NewMenuClient(IExplorerplusplus *pexpp);

	// INewMenuClient
	IFACEMETHODIMP IncludeItems(NMCII_FLAGS *flags);
	IFACEMETHODIMP SelectAndEditItem(PCIDLIST_ABSOLUTE pidlItem, NMCSAEI_FLAGS flags);

private:
	// From testing, this value is sent on Windows 10 when an item needs to be put into rename mode.
	static const int NMCSAEI_EDIT_WINDOWS_10 = 0x0003;

	IExplorerplusplus *m_pexpp;
};
