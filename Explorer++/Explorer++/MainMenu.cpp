// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Explorer++_internal.h"
#include "Icon.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/ShellHelper.h"
#include <wil/resource.h>
#include <map>

// clang-format off
const std::map<UINT, Icon> MAIN_MENU_IMAGE_MAPPINGS = {
	{ IDM_FILE_NEWTAB, Icon::NewTab},
	{ IDM_FILE_CLOSETAB, Icon::CloseTab },
	{ IDM_FILE_OPENCOMMANDPROMPT, Icon::CommandLine },
	{ IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR, Icon::CommandLineAdmin },
	{ IDM_FILE_DELETE, Icon::Delete },
	{ IDM_FILE_DELETEPERMANENTLY, Icon::DeletePermanently },
	{ IDM_FILE_RENAME, Icon::Rename },
	{ IDM_FILE_PROPERTIES, Icon::Properties },

	{ IDM_EDIT_UNDO, Icon::Undo },
	{ IDM_EDIT_COPY, Icon::Copy },
	{ IDM_EDIT_CUT, Icon::Cut },
	{ IDM_EDIT_PASTE, Icon::Paste },
	{ IDM_EDIT_PASTESHORTCUT, Icon::PasteShortcut },

	{ IDM_EDIT_COPYTOFOLDER, Icon::CopyTo },
	{ IDM_EDIT_MOVETOFOLDER, Icon::MoveTo },

	{ IDM_ACTIONS_NEWFOLDER, Icon::NewFolder },
	{ IDM_ACTIONS_SPLITFILE, Icon::SplitFiles },
	{ IDM_ACTIONS_MERGEFILES, Icon::MergeFiles },

	{ IDM_VIEW_REFRESH, Icon::Refresh },
	{ IDM_VIEW_SELECTCOLUMNS, Icon::SelectColumns },

	{ IDM_FILTER_FILTERRESULTS, Icon::Filter },

	{ IDM_GO_BACK, Icon::Back },
	{ IDM_GO_FORWARD, Icon::Forward },
	{ IDM_GO_UPONELEVEL, Icon::Up },

	{ IDM_TOOLS_SEARCH, Icon::Search },
	{ IDM_TOOLS_CUSTOMIZECOLORS, Icon::CustomizeColors },
	{ IDM_TOOLS_OPTIONS, Icon::Options },

	{ IDM_HELP_HELP, Icon::Help }
};
// clang-format on

void Explorerplusplus::InitializeMainMenu()
{
	// These need to occur after the language module has been initialized, but
	// before the tabs are restored.
	HMENU mainMenu = LoadMenu(m_hLanguageModule, MAKEINTRESOURCE(IDR_MAINMENU));

	if (!g_enablePlugins)
	{
		DeleteMenu(mainMenu, IDM_TOOLS_RUNSCRIPT, MF_BYCOMMAND);
	}

	SetMenu(m_hContainer, mainMenu);

	AddViewModesToMenu(mainMenu);

	DeleteMenu(mainMenu, IDM_VIEW_PLACEHOLDER, MF_BYCOMMAND);

	SetMainMenuImages();

	SetGoMenuName(mainMenu, IDM_GO_MYCOMPUTER, CSIDL_DRIVES);
	SetGoMenuName(mainMenu, IDM_GO_MYDOCUMENTS, CSIDL_PERSONAL);
	SetGoMenuName(mainMenu, IDM_GO_MYMUSIC, CSIDL_MYMUSIC);
	SetGoMenuName(mainMenu, IDM_GO_MYPICTURES, CSIDL_MYPICTURES);
	SetGoMenuName(mainMenu, IDM_GO_DESKTOP, CSIDL_DESKTOP);
	SetGoMenuName(mainMenu, IDM_GO_RECYCLEBIN, CSIDL_BITBUCKET);
	SetGoMenuName(mainMenu, IDM_GO_CONTROLPANEL, CSIDL_CONTROLS);
	SetGoMenuName(mainMenu, IDM_GO_PRINTERS, CSIDL_PRINTERS);
	SetGoMenuName(mainMenu, IDM_GO_CDBURNING, CSIDL_CDBURN_AREA);
	SetGoMenuName(mainMenu, IDM_GO_MYNETWORKPLACES, CSIDL_NETWORK);
	SetGoMenuName(mainMenu, IDM_GO_NETWORKCONNECTIONS, CSIDL_CONNECTIONS);
}

void Explorerplusplus::SetMainMenuImages()
{
	HMENU mainMenu = GetMenu(m_hContainer);
	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(m_hContainer);

	for (const auto &mapping : MAIN_MENU_IMAGE_MAPPINGS)
	{
		ResourceHelper::SetMenuItemImage(
			mainMenu, mapping.first, m_iconResourceLoader.get(), mapping.second, dpi, m_menuImages);
	}
}

void Explorerplusplus::SetGoMenuName(HMENU hMenu, UINT uMenuID, UINT csidl)
{
	unique_pidl_absolute pidl;
	HRESULT hr = SHGetFolderLocation(nullptr, csidl, nullptr, 0, wil::out_param(pidl));

	/* Don't use SUCCEEDED(hr). */
	if (hr == S_OK)
	{
		std::wstring folderName;
		hr = GetDisplayName(pidl.get(), SHGDN_INFOLDER, folderName);

		if (SUCCEEDED(hr))
		{
			MENUITEMINFO mii;
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_STRING;
			mii.dwTypeData = folderName.data();
			SetMenuItemInfo(hMenu, uMenuID, FALSE, &mii);

			return;
		}
	}

	DeleteMenu(hMenu, uMenuID, MF_BYCOMMAND);
}

boost::signals2::connection Explorerplusplus::AddMainMenuPreShowObserver(
	const MainMenuPreShowSignal::slot_type &observer)
{
	return m_mainMenuPreShowSignal.connect(observer);
}