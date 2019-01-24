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