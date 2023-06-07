// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "HolderWindow.h"
#include "IconResourceLoader.h"
#include "MainToolbar.h"
#include "MainToolbarButtons.h"
#include "ShellTreeView/ShellTreeView.h"
#include "TabContainer.h"
#include "../Helper/Controls.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/WindowHelper.h"

HWND Explorerplusplus::CreateTabToolbar(HWND hParent, int idCommand, const std::wstring &tip)
{
	HWND tabToolbar = CreateToolbar(hParent,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS | TBSTYLE_LIST
			| TBSTYLE_TRANSPARENT | TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE,
		TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DOUBLEBUFFER);

	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(tabToolbar);
	int scaledIconSize = MulDiv(16, dpi, USER_DEFAULT_SCREEN_DPI);

	SendMessage(tabToolbar, TB_SETBITMAPSIZE, 0, MAKELONG(scaledIconSize, scaledIconSize));
	SendMessage(tabToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	SendMessage(tabToolbar, TB_SETBUTTONSIZE, 0, MAKELPARAM(scaledIconSize, scaledIconSize));

	/* TODO: The image list is been leaked. */
	HIMAGELIST himl =
		ImageList_Create(scaledIconSize, scaledIconSize, ILC_COLOR32 | ILC_MASK, 0, 1);
	wil::unique_hbitmap bitmap =
		m_iconResourceLoader->LoadBitmapFromPNGForDpi(Icon::CloseButton, 16, 16, dpi);
	int iIndex = ImageList_Add(himl, bitmap.get(), nullptr);
	SendMessage(tabToolbar, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(himl));

	/* Add the close button, used to close tabs. */
	TBBUTTON tbButton;
	tbButton.iBitmap = iIndex;
	tbButton.idCommand = idCommand;
	tbButton.fsState = TBSTATE_ENABLED;
	tbButton.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;
	tbButton.dwData = 0;
	tbButton.iString = reinterpret_cast<INT_PTR>(tip.c_str());
	SendMessage(tabToolbar, TB_INSERTBUTTON, 0, reinterpret_cast<LPARAM>(&tbButton));

	SendMessage(tabToolbar, TB_AUTOSIZE, 0, 0);

	return tabToolbar;
}

void Explorerplusplus::ResizeWindows()
{
	RECT rc;

	GetClientRect(m_hContainer, &rc);
	SendMessage(m_hContainer, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
}

/* TODO: This should be linked to OnSize(). */
void Explorerplusplus::SetListViewInitialPosition(HWND hListView)
{
	RECT rc;
	int mainWindowWidth;
	int mainWindowHeight;
	int indentBottom = 0;
	int indentTop = 0;
	int indentLeft = 0;
	int indentRight = 0;
	int indentRebar = 0;

	GetClientRect(m_hContainer, &rc);

	mainWindowWidth = GetRectWidth(&rc);
	mainWindowHeight = GetRectHeight(&rc);

	if (m_hMainRebar)
	{
		GetWindowRect(m_hMainRebar, &rc);
		indentRebar += GetRectHeight(&rc);
	}

	if (m_config->showStatusBar)
	{
		GetWindowRect(m_hStatusBar, &rc);
		indentBottom += GetRectHeight(&rc);
	}

	if (m_config->showDisplayWindow)
	{
		if (m_config->displayWindowVertical)
		{
			indentRight += m_config->displayWindowWidth;
		}
		else
		{
			indentBottom += m_config->displayWindowHeight;
		}
	}

	if (m_config->showFolders)
	{
		GetClientRect(m_treeViewHolder->GetHWND(), &rc);
		indentLeft = GetRectWidth(&rc);
	}

	indentTop = indentRebar;

	RECT tabWindowRect;
	GetClientRect(m_tabContainer->GetHWND(), &tabWindowRect);

	int tabWindowHeight = GetRectHeight(&tabWindowRect);

	if (m_bShowTabBar)
	{
		if (!m_config->showTabBarAtBottom.get())
		{
			indentTop += tabWindowHeight;
		}
	}

	int width = mainWindowWidth - indentLeft - indentRight;
	int height = mainWindowHeight - indentTop - indentBottom;

	if (m_config->showTabBarAtBottom.get())
	{
		height -= tabWindowHeight;
	}

	SetWindowPos(hListView, nullptr, indentLeft, indentTop, width, height,
		SWP_HIDEWINDOW | SWP_NOZORDER);
}

void Explorerplusplus::ToggleFolders()
{
	m_config->showFolders = !m_config->showFolders;

	if (m_config->showFolders)
	{
		UpdateTreeViewSelection();
	}

	lShowWindow(m_treeViewHolder->GetHWND(), m_config->showFolders);
	lShowWindow(m_shellTreeView->GetHWND(), m_config->showFolders);

	SendMessage(m_mainToolbar->GetHWND(), TB_CHECKBUTTON, (WPARAM) MainToolbarButton::Folders,
		m_config->showFolders);
	ResizeWindows();
}

void Explorerplusplus::UpdateLayout()
{
	RECT rc;
	GetClientRect(m_hContainer, &rc);
	SendMessage(m_hContainer, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
}
