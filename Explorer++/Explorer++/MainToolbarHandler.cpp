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
#include "DefaultToolbarButtons.h"
#include "Explorer++_internal.h"
#include "MainImages.h"
#include "MainResource.h"
#include "../Helper/Controls.h"
#include "../Helper/Macros.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/ShellHelper.h"


void Explorerplusplus::UpdateMainToolbar()
{
	BOOL bVirtualFolder = m_pActiveShellBrowser->InVirtualFolder();

	SendMessage(m_mainToolbar->GetHWND(),TB_ENABLEBUTTON,TOOLBAR_UP,m_pActiveShellBrowser->CanBrowseUp());

	SendMessage(m_mainToolbar->GetHWND(),TB_ENABLEBUTTON,TOOLBAR_BACK,m_pActiveShellBrowser->IsBackHistory());

	SendMessage(m_mainToolbar->GetHWND(),TB_ENABLEBUTTON,TOOLBAR_FORWARD,m_pActiveShellBrowser->IsForwardHistory());

	SendMessage(m_mainToolbar->GetHWND(),TB_ENABLEBUTTON,(WPARAM)TOOLBAR_COPYTO,IsCutOrCopyPossible() && GetFocus() != m_hTreeView);
	SendMessage(m_mainToolbar->GetHWND(),TB_ENABLEBUTTON,(WPARAM)TOOLBAR_MOVETO,IsCutOrCopyPossible() && GetFocus() != m_hTreeView);
	SendMessage(m_mainToolbar->GetHWND(),TB_ENABLEBUTTON,(WPARAM)TOOLBAR_COPY,IsCutOrCopyPossible());
	SendMessage(m_mainToolbar->GetHWND(),TB_ENABLEBUTTON,(WPARAM)TOOLBAR_CUT,IsCutOrCopyPossible());
	SendMessage(m_mainToolbar->GetHWND(),TB_ENABLEBUTTON,(WPARAM)TOOLBAR_PASTE,CanPaste());
	SendMessage(m_mainToolbar->GetHWND(),TB_ENABLEBUTTON,(WPARAM)TOOLBAR_PROPERTIES,CanShowFileProperties());
	SendMessage(m_mainToolbar->GetHWND(),TB_ENABLEBUTTON,(WPARAM)TOOLBAR_DELETE,IsDeletionPossible());
	SendMessage(m_mainToolbar->GetHWND(),TB_ENABLEBUTTON,(WPARAM)TOOLBAR_DELETEPERMANENTLY,IsDeletionPossible());

	SendMessage(m_mainToolbar->GetHWND(),TB_ENABLEBUTTON,(WPARAM)TOOLBAR_OPENCOMMANDPROMPT,!bVirtualFolder);

	SendMessage(m_mainToolbar->GetHWND(),TB_ENABLEBUTTON,TOOLBAR_NEWFOLDER,!bVirtualFolder);
}

void Explorerplusplus::OnMainToolbarRClick()
{
	POINT ptCursor;
	DWORD dwPos;

	lCheckMenuItem(m_hToolbarRightClickMenu, IDM_TOOLBARS_ADDRESSBAR, m_config->showAddressBar);
	lCheckMenuItem(m_hToolbarRightClickMenu, IDM_TOOLBARS_MAINTOOLBAR, m_config->showMainToolbar);
	lCheckMenuItem(m_hToolbarRightClickMenu, IDM_TOOLBARS_BOOKMARKSTOOLBAR, m_config->showBookmarksToolbar);
	lCheckMenuItem(m_hToolbarRightClickMenu, IDM_TOOLBARS_DRIVES, m_config->showDrivesToolbar);
	lCheckMenuItem(m_hToolbarRightClickMenu, IDM_TOOLBARS_APPLICATIONTOOLBAR, m_config->showApplicationToolbar);
	lCheckMenuItem(m_hToolbarRightClickMenu, IDM_TOOLBARS_LOCKTOOLBARS, m_bLockToolbars);

	SetFocus(m_mainToolbar->GetHWND());
	dwPos = GetMessagePos();
	ptCursor.x = GET_X_LPARAM(dwPos);
	ptCursor.y = GET_Y_LPARAM(dwPos);

	TrackPopupMenu(m_hToolbarRightClickMenu, TPM_LEFTALIGN,
		ptCursor.x, ptCursor.y, 0, m_hMainRebar, NULL);
}