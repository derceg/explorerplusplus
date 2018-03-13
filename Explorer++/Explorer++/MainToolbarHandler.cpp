/******************************************************************
 *
 * Project: Explorer++
 * File: MainToolbarHandler.cpp
 * License: GPL - See LICENSE in the top level directory
 *
 * Handles functionality associated with the main toolbar.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "Explorer++_internal.h"
#include "MainImages.h"
#include "DefaultToolbarButtons.h"
#include "MainResource.h"
#include "../Helper/Controls.h"
#include "../Helper/Macros.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/ShellHelper.h"


void Explorerplusplus::CreateMainToolbar()
{
	m_hMainToolbar = CreateToolbar(m_hMainRebar, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
		TBSTYLE_TOOLTIPS | TBSTYLE_LIST | TBSTYLE_TRANSPARENT | TBSTYLE_FLAT | CCS_NODIVIDER |
		CCS_NORESIZE | CCS_ADJUSTABLE, TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS |
		TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_HIDECLIPPEDBUTTONS);

	HIMAGELIST *phiml = NULL;
	int cx;
	int cy;

	if(m_bLargeToolbarIcons)
	{
		cx = TOOLBAR_IMAGE_SIZE_LARGE_X;
		cy = TOOLBAR_IMAGE_SIZE_LARGE_Y;
		phiml = &m_himlToolbarLarge;
	}
	else
	{
		cx = TOOLBAR_IMAGE_SIZE_SMALL_X;
		cy = TOOLBAR_IMAGE_SIZE_SMALL_Y;
		phiml = &m_himlToolbarSmall;
	}

	SendMessage(m_hMainToolbar, TB_SETBITMAPSIZE, 0, MAKELONG(cx, cy));
	SendMessage(m_hMainToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

	/* Add the custom buttons to the toolbars image list. */
	SendMessage(m_hMainToolbar, TB_SETIMAGELIST, 0, (LPARAM) *phiml);

	AddStringsToMainToolbar();
	AddButtonsToMainToolbar();

	if(!m_bLoadSettingsFromXML)
	{
		if(m_bAttemptToolbarRestore)
		{
			TBSAVEPARAMS	tbSave;

			tbSave.hkr = HKEY_CURRENT_USER;
			tbSave.pszSubKey = NExplorerplusplus::REG_SETTINGS_KEY;
			tbSave.pszValueName = _T("ToolbarState");

			SendMessage(m_hMainToolbar, TB_SAVERESTORE, FALSE, (LPARAM) &tbSave);
		}
	}
}

void Explorerplusplus::SetInitialToolbarButtons()
{
	m_tbInitial = std::list<ToolbarButton_t>(DEFAULT_TOOLBAR_BUTTONS,
		DEFAULT_TOOLBAR_BUTTONS + SIZEOF_ARRAY(DEFAULT_TOOLBAR_BUTTONS));
}

void Explorerplusplus::AddButtonsToMainToolbar()
{
	for each (auto toolbarButton in m_tbInitial)
	{
		AddButtonToMainToolbar(toolbarButton.iItemID);
	}
}

void Explorerplusplus::AddButtonToMainToolbar(int iButtonId)
{
	TBBUTTON tbButton = GetMainToolbarButtonDetails(iButtonId);
	SendMessage(m_hMainToolbar, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&tbButton));
}

TBBUTTON Explorerplusplus::GetMainToolbarButtonDetails(int iButtonId)
{
	TBBUTTON tbButton;

	ZeroMemory(&tbButton, sizeof(tbButton));

	if(iButtonId == TOOLBAR_SEPARATOR)
	{
		tbButton.iBitmap = 0;
		tbButton.idCommand = 0;
		tbButton.fsState = TBSTATE_ENABLED;
		tbButton.fsStyle = BTNS_SEP;
		tbButton.dwData = 0;
		tbButton.iString = 0;
	}
	else
	{
		/* Standard style that all toolbar buttons will have. */
		BYTE StandardStyle = BTNS_BUTTON | BTNS_AUTOSIZE;

		auto itr = m_mainToolbarStringMap.find(iButtonId);
		assert(itr != m_mainToolbarStringMap.end());

		int stringIndex = itr->second;

		tbButton.iBitmap = LookupToolbarButtonImage(iButtonId);
		tbButton.idCommand = iButtonId;
		tbButton.fsState = TBSTATE_ENABLED;
		tbButton.fsStyle = StandardStyle | LookupToolbarButtonExtraStyles(iButtonId);
		tbButton.dwData = 0;
		tbButton.iString = stringIndex;
	}

	return tbButton;
}

void Explorerplusplus::AddStringsToMainToolbar()
{
	for(int i = 0; i < SIZEOF_ARRAY(TOOLBAR_BUTTON_SET); i++)
	{
		AddStringToMainToolbar(TOOLBAR_BUTTON_SET[i]);
	}
}

void Explorerplusplus::AddStringToMainToolbar(int iButtonId)
{
	TCHAR szText[64];

	/* The string must be double NULL-terminated. */
	GetMainToolbarButtonText(iButtonId, szText, SIZEOF_ARRAY(szText));
	szText[lstrlen(szText) + 1] = '\0';

	int index = static_cast<int>(SendMessage(m_hMainToolbar, TB_ADDSTRING, NULL, reinterpret_cast<LPARAM>(szText)));

	m_mainToolbarStringMap.insert(std::make_pair(iButtonId, index));
}

void Explorerplusplus::GetMainToolbarButtonText(int iButtonId, TCHAR *szText, int bufSize)
{
	int res = LoadString(m_hLanguageModule, LookupToolbarButtonTextID(iButtonId), szText, bufSize);
	assert(res != 0);

	/* It doesn't really make sense to return this. If the string isn't in the
	string table, there's a bug somewhere in the program. */
	UNUSED(res);
}

int Explorerplusplus::LookupToolbarButtonImage(int iButtonID)
{
	switch(iButtonID)
	{
	case TOOLBAR_SEPARATOR:
		return -1;
		break;

	case TOOLBAR_BACK:
		return SHELLIMAGES_BACK;
		break;

	case TOOLBAR_FORWARD:
		return SHELLIMAGES_FORWARD;
		break;

	case TOOLBAR_UP:
		return SHELLIMAGES_UP;
		break;

	case TOOLBAR_FOLDERS:
		return SHELLIMAGES_FOLDERS;
		break;

	case TOOLBAR_COPYTO:
		return SHELLIMAGES_COPYTO;
		break;

	case TOOLBAR_MOVETO:
		return SHELLIMAGES_MOVETO;
		break;

	case TOOLBAR_NEWFOLDER:
		return SHELLIMAGES_NEWFOLDER;
		break;

	case TOOLBAR_COPY:
		return SHELLIMAGES_COPY;
		break;

	case TOOLBAR_CUT:
		return SHELLIMAGES_CUT;
		break;

	case TOOLBAR_PASTE:
		return SHELLIMAGES_PASTE;
		break;

	case TOOLBAR_DELETE:
		return SHELLIMAGES_DELETE;
		break;

	case TOOLBAR_DELETEPERMANENTLY:
		return SHELLIMAGES_DELETEPERMANENTLY;
		break;

	case TOOLBAR_VIEWS:
		return SHELLIMAGES_VIEWS;
		break;

	case TOOLBAR_SEARCH:
		return SHELLIMAGES_SEARCH;
		break;

	case TOOLBAR_PROPERTIES:
		return SHELLIMAGES_PROPERTIES;
		break;

	case TOOLBAR_REFRESH:
		return SHELLIMAGES_REFRESH;
		break;

	case TOOLBAR_ADDBOOKMARK:
		return SHELLIMAGES_ADDFAV;
		break;

	case TOOLBAR_ORGANIZEBOOKMARKS:
		return SHELLIMAGES_FAV;
		break;

	case TOOLBAR_NEWTAB:
		return SHELLIMAGES_NEWTAB;
		break;

	case TOOLBAR_OPENCOMMANDPROMPT:
		return SHELLIMAGES_CMD;
		break;
	}

	return -1;
}

BYTE Explorerplusplus::LookupToolbarButtonExtraStyles(int iButtonID)
{
	switch(iButtonID)
	{
	case TOOLBAR_SEPARATOR:
		return 0;
		break;

	case TOOLBAR_BACK:
		return BTNS_DROPDOWN;
		break;

	case TOOLBAR_FORWARD:
		return BTNS_DROPDOWN;
		break;

	case TOOLBAR_UP:
		return 0;
		break;

	case TOOLBAR_FOLDERS:
		return BTNS_SHOWTEXT | BTNS_CHECK;
		break;

	case TOOLBAR_COPYTO:
		return 0;
		break;

	case TOOLBAR_MOVETO:
		return 0;
		break;

	case TOOLBAR_NEWFOLDER:
		return 0;
		break;

	case TOOLBAR_COPY:
		return 0;
		break;

	case TOOLBAR_CUT:
		return 0;
		break;

	case TOOLBAR_PASTE:
		return 0;
		break;

	case TOOLBAR_DELETE:
		return 0;
		break;

	case TOOLBAR_DELETEPERMANENTLY:
		return 0;
		break;

	case TOOLBAR_VIEWS:
		return BTNS_DROPDOWN;
		break;

	case TOOLBAR_SEARCH:
		return 0;
		break;

	case TOOLBAR_PROPERTIES:
		return 0;
		break;

	case TOOLBAR_REFRESH:
		return 0;
		break;

	case TOOLBAR_ADDBOOKMARK:
		return 0;
		break;

	case TOOLBAR_ORGANIZEBOOKMARKS:
		return 0;
		break;

	case TOOLBAR_NEWTAB:
		return 0;
		break;

	case TOOLBAR_OPENCOMMANDPROMPT:
		return 0;
		break;
	}

	return 0;
}

int Explorerplusplus::LookupToolbarButtonTextID(int iButtonID)
{
	switch(iButtonID)
	{
	case TOOLBAR_SEPARATOR:
		return IDS_SEPARATOR;
		break;

	case TOOLBAR_BACK:
		return IDS_TOOLBAR_BACK;
		break;

	case TOOLBAR_FORWARD:
		return IDS_TOOLBAR_FORWARD;
		break;

	case TOOLBAR_UP:
		return IDS_TOOLBAR_UP;
		break;

	case TOOLBAR_FOLDERS:
		return IDS_TOOLBAR_FOLDERS;
		break;

	case TOOLBAR_COPYTO:
		return IDS_TOOLBAR_COPYTO;
		break;

	case TOOLBAR_MOVETO:
		return IDS_TOOLBAR_MOVETO;
		break;

	case TOOLBAR_NEWFOLDER:
		return IDS_TOOLBAR_NEWFOLDER;
		break;

	case TOOLBAR_COPY:
		return IDS_TOOLBAR_COPY;
		break;

	case TOOLBAR_CUT:
		return IDS_TOOLBAR_CUT;
		break;

	case TOOLBAR_PASTE:
		return IDS_TOOLBAR_PASTE;
		break;

	case TOOLBAR_DELETE:
		return IDS_TOOLBAR_DELETE;
		break;

	case TOOLBAR_DELETEPERMANENTLY:
		return IDS_TOOLBAR_DELETEPERMANENTLY;
		break;

	case TOOLBAR_VIEWS:
		return IDS_TOOLBAR_VIEWS;
		break;

	case TOOLBAR_SEARCH:
		return IDS_TOOLBAR_SEARCH;
		break;

	case TOOLBAR_PROPERTIES:
		return IDS_TOOLBAR_PROPERTIES;
		break;

	case TOOLBAR_REFRESH:
		return IDS_TOOLBAR_REFRESH;
		break;

	case TOOLBAR_ADDBOOKMARK:
		return IDS_TOOLBAR_ADDBOOKMARK;
		break;

	case TOOLBAR_ORGANIZEBOOKMARKS:
		return IDS_TOOLBAR_MANAGEBOOKMARKS;
		break;

	case TOOLBAR_NEWTAB:
		return IDS_TOOLBAR_NEWTAB;
		break;

	case TOOLBAR_OPENCOMMANDPROMPT:
		return IDS_TOOLBAR_OPENCOMMANDPROMPT;
		break;
	}

	return 0;
}

void Explorerplusplus::UpdateMainToolbar(void)
{
	BOOL bVirtualFolder = m_pActiveShellBrowser->InVirtualFolder();

	SendMessage(m_hMainToolbar,TB_ENABLEBUTTON,TOOLBAR_UP,m_pActiveShellBrowser->CanBrowseUp());

	SendMessage(m_hMainToolbar,TB_ENABLEBUTTON,TOOLBAR_BACK,m_pActiveShellBrowser->IsBackHistory());

	SendMessage(m_hMainToolbar,TB_ENABLEBUTTON,TOOLBAR_FORWARD,m_pActiveShellBrowser->IsForwardHistory());

	SendMessage(m_hMainToolbar,TB_ENABLEBUTTON,(WPARAM)TOOLBAR_COPYTO,CanCutOrCopySelection() && GetFocus() != m_hTreeView);
	SendMessage(m_hMainToolbar,TB_ENABLEBUTTON,(WPARAM)TOOLBAR_MOVETO,CanCutOrCopySelection() && GetFocus() != m_hTreeView);
	SendMessage(m_hMainToolbar,TB_ENABLEBUTTON,(WPARAM)TOOLBAR_COPY,CanCutOrCopySelection());
	SendMessage(m_hMainToolbar,TB_ENABLEBUTTON,(WPARAM)TOOLBAR_CUT,CanCutOrCopySelection());
	SendMessage(m_hMainToolbar,TB_ENABLEBUTTON,(WPARAM)TOOLBAR_PASTE,CanPaste());
	SendMessage(m_hMainToolbar,TB_ENABLEBUTTON,(WPARAM)TOOLBAR_PROPERTIES,CanShowFileProperties());
	SendMessage(m_hMainToolbar,TB_ENABLEBUTTON,(WPARAM)TOOLBAR_DELETE,IsDeletionPossible());
	SendMessage(m_hMainToolbar,TB_ENABLEBUTTON,(WPARAM)TOOLBAR_DELETEPERMANENTLY,IsDeletionPossible());

	SendMessage(m_hMainToolbar,TB_ENABLEBUTTON,(WPARAM)TOOLBAR_OPENCOMMANDPROMPT,!bVirtualFolder);

	SendMessage(m_hMainToolbar,TB_ENABLEBUTTON,TOOLBAR_NEWFOLDER,!bVirtualFolder);
}

BOOL Explorerplusplus::OnTBQueryInsert()
{
	return TRUE;
}

BOOL Explorerplusplus::OnTBQueryDelete()
{
	/* All buttons can be deleted. */
	return TRUE;
}

BOOL Explorerplusplus::OnTBRestore()
{
	return 0;
}

/* This function is the reason why the toolbar string pool is used. When
customizing the toolbar, the text assigned to pszText is used. However, when
restoring the toolbar (via TB_SAVERESTORE), a TBN_GETBUTTONINFO notification is
sent for each button, and the iString parameter must be set to a valid string or
index. */
BOOL Explorerplusplus::OnTBGetButtonInfo(LPARAM lParam)
{
	NMTOOLBAR *pnmtb = reinterpret_cast<NMTOOLBAR *>(lParam);

	/* The cast below is to fix C4018 (signed/unsigned mismatch). */
	if((pnmtb->iItem >= 0) && ((unsigned int) pnmtb->iItem < SIZEOF_ARRAY(TOOLBAR_BUTTON_SET)))
	{
		int iButtonId = TOOLBAR_BUTTON_SET[pnmtb->iItem];

		pnmtb->tbButton = GetMainToolbarButtonDetails(iButtonId);

		TCHAR szText[64];
		GetMainToolbarButtonText(iButtonId, szText, SIZEOF_ARRAY(szText));
		StringCchCopy(pnmtb->pszText, pnmtb->cchText, szText);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void Explorerplusplus::OnTBReset(void)
{
	int nButtons;
	int i = 0;

	nButtons = (int) SendMessage(m_hMainToolbar, TB_BUTTONCOUNT, 0, 0);

	for(i = nButtons - 1; i >= 0; i--)
		SendMessage(m_hMainToolbar, TB_DELETEBUTTON, i, 0);

	AddButtonsToMainToolbar();
	UpdateMainToolbar();
}

void Explorerplusplus::OnTBGetInfoTip(LPARAM lParam)
{
	NMTBGETINFOTIP *ptbgit = reinterpret_cast<NMTBGETINFOTIP *>(lParam);

	StringCchCopy(ptbgit->pszText, ptbgit->cchTextMax, EMPTY_STRING);

	if(ptbgit->iItem == TOOLBAR_BACK)
	{
		if(m_pActiveShellBrowser->IsBackHistory())
		{
			LPITEMIDLIST pidl = m_pActiveShellBrowser->RetrieveHistoryItemWithoutUpdate(-1);

			TCHAR szPath[MAX_PATH];
			GetDisplayName(pidl, szPath, SIZEOF_ARRAY(szPath), SHGDN_INFOLDER);

			CoTaskMemFree(pidl);

			TCHAR szInfoTip[1024];
			TCHAR szTemp[64];
			LoadString(m_hLanguageModule, IDS_MAIN_TOOLBAR_BACK, szTemp, SIZEOF_ARRAY(szTemp));
			StringCchPrintf(szInfoTip, SIZEOF_ARRAY(szInfoTip), szTemp, szPath);

			StringCchCopy(ptbgit->pszText, ptbgit->cchTextMax, szInfoTip);
		}
	}
	else if(ptbgit->iItem == TOOLBAR_FORWARD)
	{
		if(m_pActiveShellBrowser->IsForwardHistory())
		{
			LPITEMIDLIST pidl = m_pActiveShellBrowser->RetrieveHistoryItemWithoutUpdate(1);

			TCHAR szPath[MAX_PATH];
			GetDisplayName(pidl, szPath, SIZEOF_ARRAY(szPath), SHGDN_INFOLDER);

			CoTaskMemFree(pidl);

			TCHAR szInfoTip[1024];
			TCHAR szTemp[64];
			LoadString(m_hLanguageModule, IDS_MAIN_TOOLBAR_FORWARD, szTemp, SIZEOF_ARRAY(szTemp));
			StringCchPrintf(szInfoTip, SIZEOF_ARRAY(szInfoTip), szTemp, szPath);

			StringCchCopy(ptbgit->pszText, ptbgit->cchTextMax, szInfoTip);
		}
	}
}

void Explorerplusplus::OnMainToolbarRClick()
{
	POINT ptCursor;
	DWORD dwPos;

	lCheckMenuItem(m_hToolbarRightClickMenu, IDM_TOOLBARS_ADDRESSBAR, m_bShowAddressBar);
	lCheckMenuItem(m_hToolbarRightClickMenu, IDM_TOOLBARS_MAINTOOLBAR, m_bShowMainToolbar);
	lCheckMenuItem(m_hToolbarRightClickMenu, IDM_TOOLBARS_BOOKMARKSTOOLBAR, m_bShowBookmarksToolbar);
	lCheckMenuItem(m_hToolbarRightClickMenu, IDM_TOOLBARS_DRIVES, m_bShowDrivesToolbar);
	lCheckMenuItem(m_hToolbarRightClickMenu, IDM_TOOLBARS_APPLICATIONTOOLBAR, m_bShowApplicationToolbar);
	lCheckMenuItem(m_hToolbarRightClickMenu, IDM_TOOLBARS_LOCKTOOLBARS, m_bLockToolbars);

	SetFocus(m_hMainToolbar);
	dwPos = GetMessagePos();
	ptCursor.x = GET_X_LPARAM(dwPos);
	ptCursor.y = GET_Y_LPARAM(dwPos);

	TrackPopupMenu(m_hToolbarRightClickMenu, TPM_LEFTALIGN,
		ptCursor.x, ptCursor.y, 0, m_hMainRebar, NULL);
}