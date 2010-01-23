/******************************************************************
 *
 * Project: Explorer++
 * File: MainToolbarHandler.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles functionality associated with the main toolbar.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"


void CContainer::SetInitialToolbarButtons(void)
{
	ToolbarButton_t	tb;

	m_tbInitial.clear();

	tb.iItemID		= TOOLBAR_BACK;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_FORWARD;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_UP;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_SEPARATOR;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_FOLDERS;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_SEPARATOR;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_COPY;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_CUT;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_PASTE;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_DELETE;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_PROPERTIES;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_SEARCH;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_SEPARATOR;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_NEWFOLDER;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_COPYTO;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_MOVETO;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_SEPARATOR;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_VIEWS;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_SHOWCOMMANDPROMPT;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_REFRESH;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_SEPARATOR;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_ADDBOOKMARK;
	m_tbInitial.push_back(tb);

	tb.iItemID		= TOOLBAR_ORGANIZEBOOKMARKS;
	m_tbInitial.push_back(tb);
}

int CContainer::LookupToolbarButtonTextID(int iButtonID)
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
		return IDS_TOOLBAR_ORGANIZEBOOKMARKS;
		break;

	case TOOLBAR_NEWTAB:
		return IDS_TOOLBAR_NEWTAB;
		break;

	case TOOLBAR_SHOWCOMMANDPROMPT:
		return IDS_TOOLBAR_SHOWCOMMANDPROMPT;
		break;
	}

	return 0;
}

int CContainer::LookupToolbarButtonImage(int iButtonID)
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

		case TOOLBAR_SHOWCOMMANDPROMPT:
			return SHELLIMAGES_CMD;
			break;
	}

	return -1;
}

BYTE CContainer::LookupToolbarButtonExtraStyles(int iButtonID)
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

		case TOOLBAR_SHOWCOMMANDPROMPT:
			return 0;
			break;
	}

	return 0;
}

void CContainer::AddStringsToMainToolbar(void)
{
	TCHAR szText[64];

	LoadString(g_hLanguageModule,LookupToolbarButtonTextID(TOOLBAR_BACK),
		szText,SIZEOF_ARRAY(szText));

	/* Double NULL terminated. */
	szText[lstrlen(szText) + 1] = '\0';
	SendMessage(m_hMainToolbar,TB_ADDSTRING,(WPARAM)0,(LPARAM)szText);

	LoadString(g_hLanguageModule,LookupToolbarButtonTextID(TOOLBAR_FORWARD),
		szText,SIZEOF_ARRAY(szText));

	/* Double NULL terminated. */
	szText[lstrlen(szText) + 1] = '\0';
	SendMessage(m_hMainToolbar,TB_ADDSTRING,(WPARAM)0,(LPARAM)szText);

	LoadString(g_hLanguageModule,LookupToolbarButtonTextID(TOOLBAR_UP),
		szText,SIZEOF_ARRAY(szText));

	/* Double NULL terminated. */
	szText[lstrlen(szText) + 1] = '\0';
	SendMessage(m_hMainToolbar,TB_ADDSTRING,(WPARAM)0,(LPARAM)szText);

	LoadString(g_hLanguageModule,LookupToolbarButtonTextID(TOOLBAR_FOLDERS),
		szText,SIZEOF_ARRAY(szText));

	/* Double NULL terminated. */
	szText[lstrlen(szText) + 1] = '\0';
	SendMessage(m_hMainToolbar,TB_ADDSTRING,(WPARAM)0,(LPARAM)szText);

	LoadString(g_hLanguageModule,LookupToolbarButtonTextID(TOOLBAR_COPYTO),
		szText,SIZEOF_ARRAY(szText));

	/* Double NULL terminated. */
	szText[lstrlen(szText) + 1] = '\0';
	SendMessage(m_hMainToolbar,TB_ADDSTRING,(WPARAM)0,(LPARAM)szText);

	LoadString(g_hLanguageModule,LookupToolbarButtonTextID(TOOLBAR_MOVETO),
		szText,SIZEOF_ARRAY(szText));

	/* Double NULL terminated. */
	szText[lstrlen(szText) + 1] = '\0';
	SendMessage(m_hMainToolbar,TB_ADDSTRING,(WPARAM)0,(LPARAM)szText);

	LoadString(g_hLanguageModule,LookupToolbarButtonTextID(TOOLBAR_NEWFOLDER),
		szText,SIZEOF_ARRAY(szText));

	/* Double NULL terminated. */
	szText[lstrlen(szText) + 1] = '\0';
	SendMessage(m_hMainToolbar,TB_ADDSTRING,(WPARAM)0,(LPARAM)szText);

	LoadString(g_hLanguageModule,LookupToolbarButtonTextID(TOOLBAR_COPY),
		szText,SIZEOF_ARRAY(szText));

	/* Double NULL terminated. */
	szText[lstrlen(szText) + 1] = '\0';
	SendMessage(m_hMainToolbar,TB_ADDSTRING,(WPARAM)0,(LPARAM)szText);

	LoadString(g_hLanguageModule,LookupToolbarButtonTextID(TOOLBAR_CUT),
		szText,SIZEOF_ARRAY(szText));

	/* Double NULL terminated. */
	szText[lstrlen(szText) + 1] = '\0';
	SendMessage(m_hMainToolbar,TB_ADDSTRING,(WPARAM)0,(LPARAM)szText);

	LoadString(g_hLanguageModule,LookupToolbarButtonTextID(TOOLBAR_PASTE),
		szText,SIZEOF_ARRAY(szText));

	/* Double NULL terminated. */
	szText[lstrlen(szText) + 1] = '\0';
	SendMessage(m_hMainToolbar,TB_ADDSTRING,(WPARAM)0,(LPARAM)szText);

	LoadString(g_hLanguageModule,LookupToolbarButtonTextID(TOOLBAR_DELETE),
		szText,SIZEOF_ARRAY(szText));

	/* Double NULL terminated. */
	szText[lstrlen(szText) + 1] = '\0';
	SendMessage(m_hMainToolbar,TB_ADDSTRING,(WPARAM)0,(LPARAM)szText);

	LoadString(g_hLanguageModule,LookupToolbarButtonTextID(TOOLBAR_VIEWS),
		szText,SIZEOF_ARRAY(szText));

	/* Double NULL terminated. */
	szText[lstrlen(szText) + 1] = '\0';
	SendMessage(m_hMainToolbar,TB_ADDSTRING,(WPARAM)0,(LPARAM)szText);

	LoadString(g_hLanguageModule,LookupToolbarButtonTextID(TOOLBAR_SEARCH),
		szText,SIZEOF_ARRAY(szText));

	/* Double NULL terminated. */
	szText[lstrlen(szText) + 1] = '\0';
	SendMessage(m_hMainToolbar,TB_ADDSTRING,(WPARAM)0,(LPARAM)szText);

	LoadString(g_hLanguageModule,LookupToolbarButtonTextID(TOOLBAR_PROPERTIES),
		szText,SIZEOF_ARRAY(szText));

	/* Double NULL terminated. */
	szText[lstrlen(szText) + 1] = '\0';
	SendMessage(m_hMainToolbar,TB_ADDSTRING,(WPARAM)0,(LPARAM)szText);

	LoadString(g_hLanguageModule,LookupToolbarButtonTextID(TOOLBAR_ADDRESSBAR_GO),
		szText,SIZEOF_ARRAY(szText));

	/* Double NULL terminated. */
	szText[lstrlen(szText) + 1] = '\0';
	SendMessage(m_hMainToolbar,TB_ADDSTRING,(WPARAM)0,(LPARAM)szText);

	LoadString(g_hLanguageModule,LookupToolbarButtonTextID(TOOLBAR_REFRESH),
		szText,SIZEOF_ARRAY(szText));

	/* Double NULL terminated. */
	szText[lstrlen(szText) + 1] = '\0';
	SendMessage(m_hMainToolbar,TB_ADDSTRING,(WPARAM)0,(LPARAM)szText);

	LoadString(g_hLanguageModule,LookupToolbarButtonTextID(TOOLBAR_ADDBOOKMARK),
		szText,SIZEOF_ARRAY(szText));

	/* Double NULL terminated. */
	szText[lstrlen(szText) + 1] = '\0';
	SendMessage(m_hMainToolbar,TB_ADDSTRING,(WPARAM)0,(LPARAM)szText);

	LoadString(g_hLanguageModule,LookupToolbarButtonTextID(TOOLBAR_NEWTAB),
		szText,SIZEOF_ARRAY(szText));

	/* Double NULL terminated. */
	szText[lstrlen(szText) + 1] = '\0';
	SendMessage(m_hMainToolbar,TB_ADDSTRING,(WPARAM)0,(LPARAM)szText);

	LoadString(g_hLanguageModule,LookupToolbarButtonTextID(TOOLBAR_SHOWCOMMANDPROMPT),
		szText,SIZEOF_ARRAY(szText));

	/* Double NULL terminated. */
	szText[lstrlen(szText) + 1] = '\0';
	SendMessage(m_hMainToolbar,TB_ADDSTRING,(WPARAM)0,(LPARAM)szText);

	LoadString(g_hLanguageModule,LookupToolbarButtonTextID(TOOLBAR_ORGANIZEBOOKMARKS),
		szText,SIZEOF_ARRAY(szText));

	/* Double NULL terminated. */
	szText[lstrlen(szText) + 1] = '\0';
	SendMessage(m_hMainToolbar,TB_ADDSTRING,(WPARAM)0,(LPARAM)szText);
}

/*
 * Sets the state of the items in the main toolbar.
 */
void CContainer::HandleToolbarItemStates(void)
{
    BOOL	bVirtualFolder;

    bVirtualFolder = m_pActiveShellBrowser->InVirtualFolder();

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

	SendMessage(m_hMainToolbar,TB_ENABLEBUTTON,(WPARAM)TOOLBAR_SHOWCOMMANDPROMPT,!bVirtualFolder);

	SendMessage(m_hMainToolbar,TB_ENABLEBUTTON,TOOLBAR_NEWFOLDER,!bVirtualFolder);
}