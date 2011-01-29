/******************************************************************
 *
 * Project: Explorer++
 * File: ApplicationToolbar.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the application toolbar.
 *
 * Notes:
 * Settings structure:
 * <ApplicationToolbar>
 *	<name="App1" command="C:\...">
 *	<name="App2" command="D:\...">
 * </ApplicationToolbar>
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "../Helper/ShellHelper.h"


DWORD ApplicationToolbarStyles	=	WS_CHILD |WS_VISIBLE |WS_CLIPSIBLINGS |WS_CLIPCHILDREN |
								TBSTYLE_TOOLTIPS | TBSTYLE_LIST | TBSTYLE_TRANSPARENT |
								TBSTYLE_FLAT | CCS_NODIVIDER| CCS_NORESIZE;

void Explorerplusplus::CreateApplicationToolbar(void)
{
	HIMAGELIST	himlSmall;

	m_hApplicationToolbar = CreateToolbar(m_hMainRebar,ApplicationToolbarStyles,
		TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DRAWDDARROWS|
		TBSTYLE_EX_DOUBLEBUFFER|TBSTYLE_EX_HIDECLIPPEDBUTTONS);

	SendMessage(m_hApplicationToolbar,TB_SETBITMAPSIZE,0,MAKELONG(16,16));
	SendMessage(m_hApplicationToolbar,TB_BUTTONSTRUCTSIZE,(WPARAM)sizeof(TBBUTTON),0);

	/* Assign the system image list to the toolbar. */
	Shell_GetImageLists(NULL,&himlSmall);
	SendMessage(m_hApplicationToolbar,TB_SETIMAGELIST,0,(LPARAM)himlSmall);

	ApplicationToolbarAddButtonsToToolbar();

	CApplicationToolbarDrop *patd = NULL;

	patd = new CApplicationToolbarDrop(this);

	RegisterDragDrop(m_hApplicationToolbar,patd);
}

void Explorerplusplus::InitializeApplicationToolbar(void)
{
	m_pAppButtons = NULL;
	m_nAppButtons = 0;
	m_iAppIdOffset = 0;
}

void Explorerplusplus::ApplicationToolbarNewButton(void)
{
	DialogBoxParam(g_hLanguageModule,
		MAKEINTRESOURCE(IDD_EDITAPPLICATIONBUTTON),
		m_hContainer,ApplicationToolbarNewButtonProcStub,(LPARAM)this);
}

ApplicationButton_t *Explorerplusplus::ApplicationToolbarAddItem(TCHAR *szName,TCHAR *szCommand,
BOOL bShowNameOnToolbar)
{
	ApplicationButton_t *pAppButton = NULL;
	ApplicationButton_t *pAppButtons = NULL;

	pAppButton = (ApplicationButton_t *)malloc(sizeof(ApplicationButton_t));

	if(pAppButton != NULL)
	{
		StringCchCopy(pAppButton->szName,
			SIZEOF_ARRAY(pAppButton->szName),
			szName);

		StringCchCopy(pAppButton->szCommand,
			SIZEOF_ARRAY(pAppButton->szCommand),
			szCommand);

		pAppButton->bShowNameOnToolbar = bShowNameOnToolbar;

		pAppButton->pNext = NULL;

		pAppButtons = m_pAppButtons;

		/* Have any buttons been added yet? */
		if(pAppButtons == NULL)
		{
			pAppButton->pPrevious = NULL;
			m_pAppButtons = pAppButton;
		}
		else
		{
			while(pAppButtons->pNext != NULL)
			{
				pAppButtons = pAppButtons->pNext;
			}

			pAppButton->pPrevious = pAppButtons;
			pAppButtons->pNext = pAppButton;
		}

		m_nAppButtons++;
	}

	return pAppButton;
}

void Explorerplusplus::ApplicationToolbarOpenItem(int iItem,TCHAR *szParameters)
{
	ApplicationButton_t	*pab = NULL;
	LPITEMIDLIST		pidl = NULL;
	TBBUTTON			tbButton;
	TCHAR				szExpandedPath[MAX_PATH];
	LRESULT				lResult;
	HRESULT				hr;

	if(iItem != -1)
	{
		lResult = SendMessage(m_hApplicationToolbar,TB_GETBUTTON,
			iItem,(LPARAM)&tbButton);

		if(lResult)
		{
			pab = (ApplicationButton_t *)tbButton.dwData;

			MyExpandEnvironmentStrings(pab->szCommand,
				szExpandedPath,SIZEOF_ARRAY(szExpandedPath));

			hr = GetIdlFromParsingName(szExpandedPath,&pidl);

			if(SUCCEEDED(hr))
			{
				OpenFileItem(pidl,szParameters);

				CoTaskMemFree(pidl);
			}
		}
	}
}

void Explorerplusplus::ApplicationToolbarShowItemProperties(int iItem)
{
	ApplicationButton_t	*pab = NULL;
	TBBUTTON			tbButton;
	LRESULT				lResult;

	if(iItem != -1)
	{
		lResult = SendMessage(m_hApplicationToolbar,TB_GETBUTTON,
			iItem,(LPARAM)&tbButton);

		if(lResult)
		{
			pab = (ApplicationButton_t *)tbButton.dwData;

			/* Record which item was selected. */
			m_pAppButtonSelected = pab;

			DialogBoxParam(g_hLanguageModule,
				MAKEINTRESOURCE(IDD_EDITAPPLICATIONBUTTON),
				m_hContainer,ApplicationButtonPropertiesProcStub,(LPARAM)this);
		}
	}
}

void Explorerplusplus::ApplicationToolbarDeleteItem(int iItem)
{
	ApplicationButton_t	*pab = NULL;
	TBBUTTON			tbButton;
	LRESULT				lResult;

	if(iItem != -1)
	{
		lResult = SendMessage(m_hApplicationToolbar,TB_GETBUTTON,
			iItem,(LPARAM)&tbButton);

		if(lResult)
		{
			TCHAR szInfoMsg[128];
			int	iMessageBoxReturn;

			LoadString(g_hLanguageModule,IDS_APPLICATIONBUTTON_DELETE,
				szInfoMsg,SIZEOF_ARRAY(szInfoMsg));

			iMessageBoxReturn = MessageBox(m_hContainer,szInfoMsg,
				NExplorerplusplus::WINDOW_NAME,MB_YESNO|MB_ICONINFORMATION|MB_DEFBUTTON2);

			if(iMessageBoxReturn == IDYES)
			{
				pab = (ApplicationButton_t *)tbButton.dwData;

				if(m_pAppButtons == pab)
					m_pAppButtons = pab->pNext;

				/* Patch the pointers. */
				if(pab->pNext != NULL)
					pab->pNext->pPrevious = pab->pPrevious;

				if(pab->pPrevious != NULL)
					pab->pPrevious->pNext = pab->pNext;

				/* Now delete the item. */
				free(pab);

				/* And remove it from the toolbar. */
				SendMessage(m_hApplicationToolbar,TB_DELETEBUTTON,iItem,0);

				UpdateToolbarBandSizing(m_hMainRebar,m_hApplicationToolbar);
			}
		}
	}
}

void Explorerplusplus::ApplicationToolbarAddButtonsToToolbar(void)
{
	ApplicationButton_t	*pAppButtons = NULL;
	TBBUTTON	*ptbButtons = NULL;
	SHFILEINFO	shfi;
	TCHAR		szExpandedPath[MAX_PATH];
	DWORD_PTR	ret;
	int			i = 0;

	pAppButtons = m_pAppButtons;

	if(m_pAppButtons != NULL)
	{
		ptbButtons = (TBBUTTON *)malloc(m_nAppButtons * sizeof(TBBUTTON));

		if(ptbButtons != NULL)
		{
			while(pAppButtons != NULL)
			{
				/* TODO: Use ApplicationToolbarAddButtonToToolbar() instead. */
				MyExpandEnvironmentStrings(pAppButtons->szCommand,
					szExpandedPath,SIZEOF_ARRAY(szExpandedPath));
				ret = SHGetFileInfo(szExpandedPath,0,&shfi,sizeof(shfi),SHGFI_SYSICONINDEX);

				/* Assign a generic icon if the file was not found. */
				if(ret == 0)
					shfi.iIcon = 0;

				ptbButtons[i].iBitmap	= shfi.iIcon;
				ptbButtons[i].idCommand	= TOOLBAR_APPLICATIONS_ID_START + m_iAppIdOffset;
				ptbButtons[i].fsState	= TBSTATE_ENABLED;
				ptbButtons[i].fsStyle	= BTNS_AUTOSIZE;
				ptbButtons[i].dwData	= (DWORD_PTR)pAppButtons;
				ptbButtons[i].iString	= NULL;

				if(pAppButtons->bShowNameOnToolbar)
				{
					ptbButtons[i].fsStyle	|= BTNS_SHOWTEXT;
					ptbButtons[i].iString	= (INT_PTR)pAppButtons->szName;
				}

				m_iAppIdOffset++;

				pAppButtons = pAppButtons->pNext;
				i++;
			}

			SendMessage(m_hApplicationToolbar,TB_ADDBUTTONS,
				(WPARAM)m_nAppButtons,(LPARAM)ptbButtons);

			UpdateToolbarBandSizing(m_hMainRebar,m_hApplicationToolbar);

			free(ptbButtons);
		}
	}
}

void Explorerplusplus::ApplicationToolbarAddButtonToToolbar(ApplicationButton_t *pab)
{
	TBBUTTON	tbButton;
	SHFILEINFO	shfi;
	TCHAR		szExpandedPath[MAX_PATH];
	DWORD_PTR	ret;

	MyExpandEnvironmentStrings(pab->szCommand,
		szExpandedPath,SIZEOF_ARRAY(szExpandedPath));
	ret = SHGetFileInfo(szExpandedPath,0,&shfi,sizeof(shfi),SHGFI_SYSICONINDEX);

	/* Assign a generic icon if the file was not found. */
	if(ret == 0)
		shfi.iIcon = 0;

	tbButton.iBitmap	= shfi.iIcon;
	tbButton.idCommand	= TOOLBAR_APPLICATIONS_ID_START + m_iAppIdOffset;
	tbButton.fsState	= TBSTATE_ENABLED;
	tbButton.fsStyle	= BTNS_AUTOSIZE;
	tbButton.dwData		= (DWORD_PTR)pab;
	tbButton.iString	= NULL;

	if(pab->bShowNameOnToolbar)
	{
		tbButton.fsStyle	|= BTNS_SHOWTEXT;
		tbButton.iString	= (INT_PTR)pab->szName;
	}

	m_iAppIdOffset++;

	SendMessage(m_hApplicationToolbar,TB_ADDBUTTONS,
		(WPARAM)1,(LPARAM)&tbButton);

	UpdateToolbarBandSizing(m_hMainRebar,m_hApplicationToolbar);
}

void Explorerplusplus::ApplicationToolbarRefreshButton(int iItem)
{
	ApplicationButton_t	*pab = NULL;
	TBBUTTON			tbButton;
	TBBUTTONINFO		tbi;
	SHFILEINFO			shfi;
	TCHAR				szExpandedPath[MAX_PATH];
	LRESULT				lResult;

	if(iItem != -1)
	{
		lResult = SendMessage(m_hApplicationToolbar,TB_GETBUTTON,
			iItem,(LPARAM)&tbButton);

		if(lResult)
		{
			pab = (ApplicationButton_t *)tbButton.dwData;

			/* If old command and new command strings are the same,
			do NOT update the buttons icon. */

			MyExpandEnvironmentStrings(pab->szCommand,
				szExpandedPath,SIZEOF_ARRAY(szExpandedPath));
			SHGetFileInfo(szExpandedPath,0,&shfi,sizeof(shfi),
				SHGFI_SYSICONINDEX);

			tbi.cbSize	= sizeof(tbi);
			tbi.dwMask	= TBIF_IMAGE|TBIF_TEXT;
			tbi.iImage	= shfi.iIcon;

			if(pab->bShowNameOnToolbar)
			{
				tbi.pszText	= pab->szName;
			}
			else
			{
				tbi.pszText = EMPTY_STRING;
			}

			SendMessage(m_hApplicationToolbar,TB_SETBUTTONINFO,tbButton.idCommand,(LPARAM)&tbi);
		}
	}
}

INT_PTR CALLBACK ApplicationButtonPropertiesProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static Explorerplusplus *pContainer = NULL;

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			pContainer = (Explorerplusplus *)lParam;
		}
		break;
	}

	return pContainer->ApplicationButtonPropertiesProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK Explorerplusplus::ApplicationButtonPropertiesProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
		case WM_INITDIALOG:
			OnApplicationButtonPropertiesInit(hDlg);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_APP_BUTTON_COMMAND:
					OnApplicationToolbarCommandButton(hDlg);
					break;

				case IDOK:
					OnApplicationButtonPropertiesOk(hDlg);
					break;

				case IDCANCEL:
					EndDialog(hDlg,0);
					break;
			}
			break;

		case WM_CLOSE:
			EndDialog(hDlg,0);
			break;
	}

	return 0;
}

void Explorerplusplus::OnApplicationButtonPropertiesInit(HWND hDlg)
{
	HWND hEditName;
	HWND hEditCommand;
	UINT uCheck;

	hEditName = GetDlgItem(hDlg,IDC_APP_EDIT_NAME);
	hEditCommand = GetDlgItem(hDlg,IDC_APP_EDIT_COMMAND);

	SetWindowText(hEditName,m_pAppButtonSelected->szName);
	SetWindowText(hEditCommand,m_pAppButtonSelected->szCommand);

	if(m_pAppButtonSelected->bShowNameOnToolbar)
		uCheck = BST_CHECKED;
	else
		uCheck = BST_UNCHECKED;

	CheckDlgButton(hDlg,IDC_CHECK_SHOWAPPNAME,uCheck);

	/* Select all the text in the 'Name' edit field. */
	SendMessage(hEditName,EM_SETSEL,0,-1);

	/* Set the focus to the 'Name' edit field. */
	SetFocus(hEditName);
}

void Explorerplusplus::OnApplicationButtonPropertiesOk(HWND hDlg)
{
	HWND	hEditName;
	HWND	hEditCommand;

	hEditName = GetDlgItem(hDlg,IDC_APP_EDIT_NAME);
	hEditCommand = GetDlgItem(hDlg,IDC_APP_EDIT_COMMAND);

	/* The name and command strings have a fixed length.
	Therefore, do not query the length of the text in
	the edit control, just get the text directly. If it
	is longer than the storage space, it will be clipped
	anyway. */
	GetWindowText(hEditName,m_pAppButtonSelected->szName,
		SIZEOF_ARRAY(m_pAppButtonSelected->szName));

	GetWindowText(hEditCommand,m_pAppButtonSelected->szCommand,
		SIZEOF_ARRAY(m_pAppButtonSelected->szCommand));

	m_pAppButtonSelected->bShowNameOnToolbar = IsDlgButtonChecked(hDlg,
		IDC_CHECK_SHOWAPPNAME) == BST_CHECKED;

	/* Refresh the button on the toolbar. */
	ApplicationToolbarRefreshButton(m_iSelectedRClick);

	EndDialog(hDlg,1);
}

INT_PTR CALLBACK ApplicationToolbarNewButtonProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static Explorerplusplus *pContainer = NULL;

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			pContainer = (Explorerplusplus *)lParam;
		}
		break;
	}

	return pContainer->ApplicationToolbarNewButtonProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK Explorerplusplus::ApplicationToolbarNewButtonProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
		case WM_INITDIALOG:
			OnApplicationToolbarNewButtonInit(hDlg);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_APP_BUTTON_COMMAND:
					OnApplicationToolbarCommandButton(hDlg);
					break;

				case IDOK:
					OnApplicationToolbarNewButtonOk(hDlg);
					break;

				case IDCANCEL:
					EndDialog(hDlg,0);
					break;
			}
			break;

		case WM_CLOSE:
			EndDialog(hDlg,0);
			break;
	}

	return 0;
}

void Explorerplusplus::OnApplicationToolbarNewButtonInit(HWND hDlg)
{
	HWND hEditName;
	TCHAR szTemp[64];

	hEditName = GetDlgItem(hDlg,IDC_APP_EDIT_NAME);

	CheckDlgButton(hDlg,IDC_CHECK_SHOWAPPNAME,BST_CHECKED);

	LoadString(g_hLanguageModule,IDS_GENERAL_NEWAPPLICATIONBUTTON,
		szTemp,SIZEOF_ARRAY(szTemp));

	SetWindowText(hDlg,szTemp);

	/* Set the focus to the 'Name' edit field. */
	SetFocus(hEditName);
}

void Explorerplusplus::OnApplicationToolbarNewButtonOk(HWND hDlg)
{
	HWND				hEditName;
	HWND				hEditCommand;
	ApplicationButton_t	*pab = NULL;
	TCHAR				szName[512];
	TCHAR				szCommand[512];

	hEditName = GetDlgItem(hDlg,IDC_APP_EDIT_NAME);
	hEditCommand = GetDlgItem(hDlg,IDC_APP_EDIT_COMMAND);

	/* The name and command strings have a fixed length.
	Therefore, do not query the length of the text in
	the edit control, just get the text directly. If it
	is longer than the storage space, it will be clipped
	anyway. */
	GetWindowText(hEditName,szName,
		SIZEOF_ARRAY(szName));

	GetWindowText(hEditCommand,szCommand,
		SIZEOF_ARRAY(szCommand));

	pab = ApplicationToolbarAddItem(szName,szCommand,
		IsDlgButtonChecked(hDlg,IDC_CHECK_SHOWAPPNAME) == BST_CHECKED);

	/* Add the application button to the toolbar. */
	ApplicationToolbarAddButtonToToolbar(pab);

	EndDialog(hDlg,1);
}

void Explorerplusplus::OnApplicationToolbarCommandButton(HWND hDlg)
{
	TCHAR *Filter = _T("Programs (*.exe)\0*.exe\0All Files\0*.*\0\0");
	OPENFILENAME ofn;
	TCHAR FullFileName[MAX_PATH] = _T("");
	BOOL bRet;
	HWND				hEditCommand;

	hEditCommand = GetDlgItem(hDlg,IDC_APP_EDIT_COMMAND);

	ofn.lStructSize			= sizeof(ofn);
	ofn.hwndOwner			= hDlg;
	ofn.lpstrFilter			= Filter;
	ofn.lpstrCustomFilter	= NULL;
	ofn.nMaxCustFilter		= 0;
	ofn.nFilterIndex		= 0;
	ofn.lpstrFile			= FullFileName;
	ofn.nMaxFile			= MAX_PATH;
	ofn.lpstrFileTitle		= NULL;
	ofn.nMaxFileTitle		= 0;
	ofn.lpstrInitialDir		= NULL;
	ofn.lpstrTitle			= NULL;
	ofn.Flags				= OFN_ENABLESIZING|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt			= _T("exe");
	ofn.lCustData			= NULL;
	ofn.lpfnHook			= NULL;
	ofn.pvReserved			= NULL;
	ofn.dwReserved			= NULL;
	ofn.FlagsEx				= NULL;

	bRet = GetOpenFileName(&ofn);

	if(bRet)
	{
		SetWindowText(hEditCommand,FullFileName);
	}
}

void Explorerplusplus::OnApplicationToolbarRClick(void)
{
	MENUITEMINFO mii;

	mii.cbSize		= sizeof(mii);
	mii.fMask		= MIIM_ID|MIIM_STRING;
	mii.dwTypeData	= _T("New Application Button...");
	mii.wID			= IDM_APP_NEW;

	/* Add the item to the menu. */
	InsertMenuItem(m_hToolbarRightClickMenu,7,TRUE,&mii);

	/* Set it to be owner drawn. */
	SetMenuItemOwnerDrawn(m_hToolbarRightClickMenu,7);

	OnMainToolbarRClick();

	mii.cbSize	= sizeof(mii);
	mii.fMask	= MIIM_DATA;
	GetMenuItemInfo(m_hToolbarRightClickMenu,7,TRUE,&mii);

	/* Free the owner drawn data. */
	free((void *)mii.dwItemData);

	/* Now, remove the item from the menu. */
	DeleteMenu(m_hToolbarRightClickMenu,7,MF_BYPOSITION);
}