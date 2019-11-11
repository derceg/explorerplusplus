// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Icon.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include <map>

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

	{ IDM_BOOKMARKS_BOOKMARKTHISTAB, Icon::AddBookmark },
	{ IDM_BOOKMARKS_MANAGEBOOKMARKS, Icon::Bookmarks },

	{ IDM_TOOLS_SEARCH, Icon::Search },
	{ IDM_TOOLS_CUSTOMIZECOLORS, Icon::CustomizeColors },
	{ IDM_TOOLS_OPTIONS, Icon::Options },

	{ IDM_HELP_HELP, Icon::Help }
};

void Explorerplusplus::InitializeMainMenu()
{
	HMENU hMenu = GetMenu(m_hContainer);

	AddViewModesToMenu(hMenu);

	/* Delete the placeholder menu. */
	DeleteMenu(hMenu, IDM_VIEW_PLACEHOLDER, MF_BYCOMMAND);

	SetMainMenuImages();

	SetGoMenuName(hMenu, IDM_GO_MYCOMPUTER, CSIDL_DRIVES);
	SetGoMenuName(hMenu, IDM_GO_MYDOCUMENTS, CSIDL_PERSONAL);
	SetGoMenuName(hMenu, IDM_GO_MYMUSIC, CSIDL_MYMUSIC);
	SetGoMenuName(hMenu, IDM_GO_MYPICTURES, CSIDL_MYPICTURES);
	SetGoMenuName(hMenu, IDM_GO_DESKTOP, CSIDL_DESKTOP);
	SetGoMenuName(hMenu, IDM_GO_RECYCLEBIN, CSIDL_BITBUCKET);
	SetGoMenuName(hMenu, IDM_GO_CONTROLPANEL, CSIDL_CONTROLS);
	SetGoMenuName(hMenu, IDM_GO_PRINTERS, CSIDL_PRINTERS);
	SetGoMenuName(hMenu, IDM_GO_CDBURNING, CSIDL_CDBURN_AREA);
	SetGoMenuName(hMenu, IDM_GO_MYNETWORKPLACES, CSIDL_NETWORK);
	SetGoMenuName(hMenu, IDM_GO_NETWORKCONNECTIONS, CSIDL_CONNECTIONS);
}

void Explorerplusplus::SetMainMenuImages()
{
	HMENU mainMenu = GetMenu(m_hContainer);
	UINT dpi = m_dpiCompat.GetDpiForWindow(m_hContainer);

	for (const auto &mapping : MAIN_MENU_IMAGE_MAPPINGS)
	{
		SetMenuItemImage(mainMenu, mapping.first, mapping.second, dpi, m_menuImages);
	}
}