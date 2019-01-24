/******************************************************************
 *
 * Project: Explorer++
 * File: MainToolbar.cpp
 * License: GPL - See LICENSE in the top level directory
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "MainToolbar.h"
#include "DefaultToolbarButtons.h"
#include "MainImages.h"
#include "MainResource.h"
#include "ShellBrowser/ViewModes.h"
#include "../Helper/Controls.h"
#include "../Helper/Macros.h"

/* Small/large toolbar image sizes. */
const int TOOLBAR_IMAGE_SIZE_SMALL_X = 16;
const int TOOLBAR_IMAGE_SIZE_SMALL_Y = 16;
const int TOOLBAR_IMAGE_SIZE_LARGE_X = 24;
const int TOOLBAR_IMAGE_SIZE_LARGE_Y = 24;

/* This represents the TOTAL set of buttons that may be
placed on the toolbar. */
static const int TOOLBAR_BUTTON_SET[] =
{ TOOLBAR_BACK, TOOLBAR_FORWARD, TOOLBAR_UP, TOOLBAR_FOLDERS,
TOOLBAR_COPYTO, TOOLBAR_MOVETO, TOOLBAR_NEWFOLDER, TOOLBAR_COPY,
TOOLBAR_CUT, TOOLBAR_PASTE, TOOLBAR_DELETE, TOOLBAR_VIEWS,
TOOLBAR_SEARCH, TOOLBAR_PROPERTIES, TOOLBAR_REFRESH,
TOOLBAR_ADDBOOKMARK, TOOLBAR_NEWTAB, TOOLBAR_OPENCOMMANDPROMPT,
TOOLBAR_ORGANIZEBOOKMARKS, TOOLBAR_DELETEPERMANENTLY };

MainToolbar *MainToolbar::Create(HWND parent, HINSTANCE instance, IExplorerplusplus *pexpp, std::shared_ptr<Config> config)
{
	return new MainToolbar(parent, instance, pexpp, config);
}

MainToolbar::MainToolbar(HWND parent, HINSTANCE instance, IExplorerplusplus *pexpp, std::shared_ptr<Config> config) :
	CBaseWindow(CreateMainToolbar(parent)),
	m_instance(instance),
	m_pexpp(pexpp),
	m_config(config)
{
	Initialize(parent);
}

MainToolbar::~MainToolbar()
{
	ImageList_Destroy(m_himlSmall);
	ImageList_Destroy(m_himlLarge);

	RemoveWindowSubclass(GetParent(m_hwnd), ParentWndProcStub, PARENT_SUBCLASS_ID);
}

HWND MainToolbar::CreateMainToolbar(HWND parent)
{
	return CreateToolbar(parent, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
		TBSTYLE_TOOLTIPS | TBSTYLE_LIST | TBSTYLE_TRANSPARENT | TBSTYLE_FLAT | CCS_NODIVIDER |
		CCS_NORESIZE | CCS_ADJUSTABLE, TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS |
		TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_HIDECLIPPEDBUTTONS);
}

void MainToolbar::Initialize(HWND parent)
{
	HBITMAP hb;

	m_himlSmall = ImageList_Create(TOOLBAR_IMAGE_SIZE_SMALL_X, TOOLBAR_IMAGE_SIZE_SMALL_Y, ILC_COLOR32 | ILC_MASK, 0, 47);
	hb = LoadBitmap(GetModuleHandle(0), MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(m_himlSmall, hb, NULL);
	DeleteObject(hb);

	m_himlLarge = ImageList_Create(TOOLBAR_IMAGE_SIZE_LARGE_X, TOOLBAR_IMAGE_SIZE_LARGE_Y, ILC_COLOR32 | ILC_MASK, 0, 47);
	hb = LoadBitmap(GetModuleHandle(0), MAKEINTRESOURCE(IDB_SHELLIMAGES_LARGE));
	ImageList_Add(m_himlLarge, hb, NULL);
	DeleteObject(hb);

	HIMAGELIST *phiml = NULL;
	int cx;
	int cy;

	if (m_config->useLargeToolbarIcons)
	{
		cx = TOOLBAR_IMAGE_SIZE_LARGE_X;
		cy = TOOLBAR_IMAGE_SIZE_LARGE_Y;
		phiml = &m_himlLarge;
	}
	else
	{
		cx = TOOLBAR_IMAGE_SIZE_SMALL_X;
		cy = TOOLBAR_IMAGE_SIZE_SMALL_Y;
		phiml = &m_himlSmall;
	}

	SendMessage(m_hwnd, TB_SETBITMAPSIZE, 0, MAKELONG(cx, cy));
	SendMessage(m_hwnd, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

	/* Add the custom buttons to the toolbars image list. */
	SendMessage(m_hwnd, TB_SETIMAGELIST, 0, (LPARAM)*phiml);

	SetInitialToolbarButtons();

	AddStringsToToolbar();
	AddButtonsToToolbar();

	/* TODO: This needs
	to be updated. */
	/*if (!m_bLoadSettingsFromXML)
	{
		if (m_bAttemptToolbarRestore)
		{
			TBSAVEPARAMS	tbSave;

			tbSave.hkr = HKEY_CURRENT_USER;
			tbSave.pszSubKey = NExplorerplusplus::REG_SETTINGS_KEY;
			tbSave.pszValueName = _T("ToolbarState");

			SendMessage(m_hMainToolbar, TB_SAVERESTORE, FALSE, (LPARAM)&tbSave);
		}
	}*/

	SetWindowSubclass(parent, ParentWndProcStub, PARENT_SUBCLASS_ID,
		reinterpret_cast<DWORD_PTR>(this));
}

LRESULT CALLBACK MainToolbar::ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	MainToolbar *mainToolbar = reinterpret_cast<MainToolbar *>(dwRefData);
	return mainToolbar->ParentWndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK MainToolbar::ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hwnd)
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case TBN_QUERYINSERT:
				return OnTBQueryInsert();
				break;

			case TBN_QUERYDELETE:
				return OnTBQueryDelete();
				break;

			case TBN_GETBUTTONINFO:
				return OnTBGetButtonInfo(lParam);
				break;

			case TBN_RESTORE:
				return OnTBRestore();
				break;

			case TBN_GETINFOTIP:
				OnTBGetInfoTip(lParam);
				break;

			case TBN_RESET:
				OnTBReset();
				break;

			case TBN_DROPDOWN:
				return OnTbnDropDown(lParam);
				break;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void MainToolbar::SetInitialToolbarButtons()
{
	m_tbInitial = std::list<ToolbarButton_t>(DEFAULT_TOOLBAR_BUTTONS,
		DEFAULT_TOOLBAR_BUTTONS + SIZEOF_ARRAY(DEFAULT_TOOLBAR_BUTTONS));
}

void MainToolbar::AddButtonsToToolbar()
{
	for (const auto &toolbarButton : m_tbInitial)
	{
		AddButtonToToolbar(toolbarButton.iItemID);
	}
}

void MainToolbar::AddButtonToToolbar(int iButtonId)
{
	TBBUTTON tbButton = GetToolbarButtonDetails(iButtonId);
	SendMessage(m_hwnd, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&tbButton));
}

TBBUTTON MainToolbar::GetToolbarButtonDetails(int iButtonId) const
{
	TBBUTTON tbButton;

	ZeroMemory(&tbButton, sizeof(tbButton));

	if (iButtonId == TOOLBAR_SEPARATOR)
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

		auto itr = m_toolbarStringMap.find(iButtonId);
		assert(itr != m_toolbarStringMap.end());

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

void MainToolbar::AddStringsToToolbar()
{
	for (int i = 0; i < SIZEOF_ARRAY(TOOLBAR_BUTTON_SET); i++)
	{
		AddStringToToolbar(TOOLBAR_BUTTON_SET[i]);
	}
}

void MainToolbar::AddStringToToolbar(int iButtonId)
{
	TCHAR szText[64];

	/* The string must be double NULL-terminated. */
	GetToolbarButtonText(iButtonId, szText, SIZEOF_ARRAY(szText));
	szText[lstrlen(szText) + 1] = '\0';

	int index = static_cast<int>(SendMessage(m_hwnd, TB_ADDSTRING, NULL, reinterpret_cast<LPARAM>(szText)));

	m_toolbarStringMap.insert(std::make_pair(iButtonId, index));
}

void MainToolbar::GetToolbarButtonText(int iButtonId, TCHAR *szText, int bufSize) const
{
	int res = LoadString(m_instance, LookupToolbarButtonTextID(iButtonId), szText, bufSize);
	assert(res != 0);

	/* It doesn't really make sense to return this. If the string isn't in the
	string table, there's a bug somewhere in the program. */
	UNUSED(res);
}

int MainToolbar::LookupToolbarButtonImage(int iButtonID) const
{
	switch (iButtonID)
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

BYTE MainToolbar::LookupToolbarButtonExtraStyles(int iButtonID) const
{
	switch (iButtonID)
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

int MainToolbar::LookupToolbarButtonTextID(int iButtonID) const
{
	switch (iButtonID)
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

void MainToolbar::UpdateToolbarSize()
{
	HIMAGELIST *phiml = NULL;
	int cx;
	int cy;

	if (m_config->useLargeToolbarIcons)
	{
		cx = TOOLBAR_IMAGE_SIZE_LARGE_X;
		cy = TOOLBAR_IMAGE_SIZE_LARGE_Y;
		phiml = &m_himlLarge;
	}
	else
	{
		cx = TOOLBAR_IMAGE_SIZE_SMALL_X;
		cy = TOOLBAR_IMAGE_SIZE_SMALL_Y;
		phiml = &m_himlSmall;
	}

	/* Switch the image list. */
	SendMessage(m_hwnd, TB_SETIMAGELIST, 0, (LPARAM)*phiml);
	SendMessage(m_hwnd, TB_SETBUTTONSIZE, 0, MAKELPARAM(cx, cy));
	SendMessage(m_hwnd, TB_AUTOSIZE, 0, 0);
}

BOOL MainToolbar::OnTBQueryInsert()
{
	return TRUE;
}

BOOL MainToolbar::OnTBQueryDelete()
{
	/* All buttons can be deleted. */
	return TRUE;
}

BOOL MainToolbar::OnTBRestore()
{
	return 0;
}

/* This function is the reason why the toolbar string pool is used. When
customizing the toolbar, the text assigned to pszText is used. However, when
restoring the toolbar (via TB_SAVERESTORE), a TBN_GETBUTTONINFO notification is
sent for each button, and the iString parameter must be set to a valid string or
index. */
BOOL MainToolbar::OnTBGetButtonInfo(LPARAM lParam)
{
	NMTOOLBAR *pnmtb = reinterpret_cast<NMTOOLBAR *>(lParam);

	/* The cast below is to fix C4018 (signed/unsigned mismatch). */
	if ((pnmtb->iItem >= 0) && ((unsigned int)pnmtb->iItem < SIZEOF_ARRAY(TOOLBAR_BUTTON_SET)))
	{
		int iButtonId = TOOLBAR_BUTTON_SET[pnmtb->iItem];

		pnmtb->tbButton = GetToolbarButtonDetails(iButtonId);

		TCHAR szText[64];
		GetToolbarButtonText(iButtonId, szText, SIZEOF_ARRAY(szText));
		StringCchCopy(pnmtb->pszText, pnmtb->cchText, szText);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void MainToolbar::OnTBReset()
{
	int nButtons;
	int i = 0;

	nButtons = (int)SendMessage(m_hwnd, TB_BUTTONCOUNT, 0, 0);

	for (i = nButtons - 1; i >= 0; i--)
		SendMessage(m_hwnd, TB_DELETEBUTTON, i, 0);

	AddButtonsToToolbar();
	UpdateToolbarButtonStates();
}

void MainToolbar::OnTBGetInfoTip(LPARAM lParam)
{
	NMTBGETINFOTIP *ptbgit = reinterpret_cast<NMTBGETINFOTIP *>(lParam);

	StringCchCopy(ptbgit->pszText, ptbgit->cchTextMax, EMPTY_STRING);

	if (ptbgit->iItem == TOOLBAR_BACK)
	{
		if (m_pexpp->GetActiveShellBrowser()->CanBrowseBack())
		{
			LPITEMIDLIST pidl = m_pexpp->GetActiveShellBrowser()->RetrieveHistoryItemWithoutUpdate(-1);

			TCHAR szPath[MAX_PATH];
			GetDisplayName(pidl, szPath, SIZEOF_ARRAY(szPath), SHGDN_INFOLDER);

			CoTaskMemFree(pidl);

			TCHAR szInfoTip[1024];
			TCHAR szTemp[64];
			LoadString(m_instance, IDS_MAIN_TOOLBAR_BACK, szTemp, SIZEOF_ARRAY(szTemp));
			StringCchPrintf(szInfoTip, SIZEOF_ARRAY(szInfoTip), szTemp, szPath);

			StringCchCopy(ptbgit->pszText, ptbgit->cchTextMax, szInfoTip);
		}
	}
	else if (ptbgit->iItem == TOOLBAR_FORWARD)
	{
		if (m_pexpp->GetActiveShellBrowser()->CanBrowseForward())
		{
			LPITEMIDLIST pidl = m_pexpp->GetActiveShellBrowser()->RetrieveHistoryItemWithoutUpdate(1);

			TCHAR szPath[MAX_PATH];
			GetDisplayName(pidl, szPath, SIZEOF_ARRAY(szPath), SHGDN_INFOLDER);

			CoTaskMemFree(pidl);

			TCHAR szInfoTip[1024];
			TCHAR szTemp[64];
			LoadString(m_instance, IDS_MAIN_TOOLBAR_FORWARD, szTemp, SIZEOF_ARRAY(szTemp));
			StringCchPrintf(szInfoTip, SIZEOF_ARRAY(szInfoTip), szTemp, szPath);

			StringCchCopy(ptbgit->pszText, ptbgit->cchTextMax, szInfoTip);
		}
	}
}

LRESULT MainToolbar::OnTbnDropDown(LPARAM lParam)
{
	NMTOOLBAR		*nmTB = NULL;
	LPITEMIDLIST	pidl = NULL;
	POINT			ptOrigin;
	RECT			rc;
	HRESULT			hr;

	nmTB = (NMTOOLBAR *)lParam;

	GetWindowRect(m_hwnd, &rc);

	ptOrigin.x = rc.left;
	ptOrigin.y = rc.bottom - 4;

	if (nmTB->iItem == TOOLBAR_BACK)
	{
		hr = m_pexpp->GetActiveShellBrowser()->CreateHistoryPopup(m_hwnd, &pidl, &ptOrigin, TRUE);

		if (SUCCEEDED(hr))
		{
			m_pexpp->BrowseFolder(pidl, SBSP_ABSOLUTE | SBSP_WRITENOHISTORY, FALSE, FALSE, FALSE);

			CoTaskMemFree(pidl);
		}

		return TBDDRET_DEFAULT;
	}
	else if (nmTB->iItem == TOOLBAR_FORWARD)
	{
		SendMessage(m_hwnd, TB_GETRECT, (WPARAM)TOOLBAR_BACK, (LPARAM)&rc);

		ptOrigin.x += rc.right;

		hr = m_pexpp->GetActiveShellBrowser()->CreateHistoryPopup(m_hwnd, &pidl, &ptOrigin, FALSE);

		if (SUCCEEDED(hr))
		{
			m_pexpp->BrowseFolder(pidl, SBSP_ABSOLUTE | SBSP_WRITENOHISTORY, FALSE, FALSE, FALSE);

			CoTaskMemFree(pidl);
		}

		return TBDDRET_DEFAULT;
	}
	else if (nmTB->iItem == TOOLBAR_VIEWS)
	{
		ShowToolbarViewsDropdown();

		return TBDDRET_DEFAULT;
	}

	return TBDDRET_NODEFAULT;
}

void MainToolbar::ShowToolbarViewsDropdown()
{
	POINT	ptOrigin;
	RECT	rcButton;

	SendMessage(m_hwnd, TB_GETRECT, (WPARAM)TOOLBAR_VIEWS, (LPARAM)&rcButton);

	ptOrigin.x = rcButton.left;
	ptOrigin.y = rcButton.bottom;

	ClientToScreen(m_hwnd, &ptOrigin);

	CreateViewsMenu(&ptOrigin);
}

void MainToolbar::CreateViewsMenu(POINT *ptOrigin)
{
	ViewMode viewMode = m_pexpp->GetActiveShellBrowser()->GetCurrentViewMode();

	HMENU viewsMenu = m_pexpp->BuildViewsMenu();

	int ItemToCheck = GetViewModeMenuId(viewMode);
	CheckMenuRadioItem(viewsMenu, IDM_VIEW_THUMBNAILS, IDM_VIEW_EXTRALARGEICONS,
		ItemToCheck, MF_BYCOMMAND);

	TrackPopupMenu(viewsMenu, TPM_LEFTALIGN, ptOrigin->x, ptOrigin->y,
		0, m_hwnd, NULL);
}

void MainToolbar::UpdateToolbarButtonStates()
{
	SendMessage(m_hwnd, TB_ENABLEBUTTON, TOOLBAR_UP, m_pexpp->GetActiveShellBrowser()->CanBrowseUp());

	SendMessage(m_hwnd, TB_ENABLEBUTTON, TOOLBAR_BACK, m_pexpp->GetActiveShellBrowser()->CanBrowseBack());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, TOOLBAR_FORWARD, m_pexpp->GetActiveShellBrowser()->CanBrowseForward());

	SendMessage(m_hwnd, TB_ENABLEBUTTON, (WPARAM)TOOLBAR_COPYTO, m_pexpp->CanCopy() && GetFocus() != m_pexpp->GetTreeView());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, (WPARAM)TOOLBAR_MOVETO, m_pexpp->CanCut() && GetFocus() != m_pexpp->GetTreeView());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, (WPARAM)TOOLBAR_COPY, m_pexpp->CanCopy());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, (WPARAM)TOOLBAR_CUT, m_pexpp->CanCut());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, (WPARAM)TOOLBAR_PASTE, m_pexpp->CanPaste());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, (WPARAM)TOOLBAR_PROPERTIES, m_pexpp->CanShowFileProperties());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, (WPARAM)TOOLBAR_DELETE, m_pexpp->CanDelete());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, (WPARAM)TOOLBAR_DELETEPERMANENTLY, m_pexpp->CanDelete());

	BOOL bVirtualFolder = m_pexpp->GetActiveShellBrowser()->InVirtualFolder();

	SendMessage(m_hwnd, TB_ENABLEBUTTON, (WPARAM)TOOLBAR_OPENCOMMANDPROMPT, !bVirtualFolder);

	SendMessage(m_hwnd, TB_ENABLEBUTTON, TOOLBAR_NEWFOLDER, !bVirtualFolder);
}