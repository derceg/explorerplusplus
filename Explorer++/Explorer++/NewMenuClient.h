// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/WinRTBaseWrapper.h"
#include <ShObjIdl.h>

class ShellBrowserImpl;

class NewMenuClient : public winrt::implements<NewMenuClient, INewMenuClient, winrt::non_agile>
{
public:
	NewMenuClient(ShellBrowserImpl *shellBrowser);

	// INewMenuClient
	IFACEMETHODIMP IncludeItems(NMCII_FLAGS *flags);
	IFACEMETHODIMP SelectAndEditItem(PCIDLIST_ABSOLUTE pidlItem, NMCSAEI_FLAGS flags);

private:
	// From testing, this value is sent on Windows 10 when an item needs to be put into rename mode.
	static const int NMCSAEI_EDIT_WINDOWS_10 = 0x0003;

	ShellBrowserImpl *m_shellBrowser;
};
