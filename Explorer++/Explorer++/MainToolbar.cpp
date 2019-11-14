// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainToolbar.h"
#include "Config.h"
#include "DefaultToolbarButtons.h"
#include "Icon.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ViewModes.h"
#include "TabContainer.h"
#include "../Helper/Controls.h"
#include "../Helper/Macros.h"
#include <gdiplus.h>

const int TOOLBAR_IMAGE_SIZE_SMALL = 16;
const int TOOLBAR_IMAGE_SIZE_LARGE = 24;

const std::unordered_map<int, Icon> TOOLBAR_BUTTON_ICON_MAPPINGS = {
	{TOOLBAR_BACK, Icon::Back},
	{TOOLBAR_FORWARD, Icon::Forward},
	{TOOLBAR_UP, Icon::Up},
	{TOOLBAR_FOLDERS, Icon::FolderTree},
	{TOOLBAR_COPYTO, Icon::CopyTo},
	{TOOLBAR_MOVETO, Icon::MoveTo},
	{TOOLBAR_NEWFOLDER, Icon::NewFolder},
	{TOOLBAR_COPY, Icon::Copy},
	{TOOLBAR_CUT, Icon::Cut},
	{TOOLBAR_PASTE, Icon::Paste},
	{TOOLBAR_DELETE, Icon::Delete},
	{TOOLBAR_VIEWS, Icon::Views},
	{TOOLBAR_SEARCH, Icon::Search},
	{TOOLBAR_PROPERTIES, Icon::Properties},
	{TOOLBAR_REFRESH, Icon::Refresh},
	{TOOLBAR_ADDBOOKMARK, Icon::AddBookmark},
	{TOOLBAR_NEWTAB, Icon::NewTab},
	{TOOLBAR_OPENCOMMANDPROMPT, Icon::CommandLine},
	{TOOLBAR_ORGANIZEBOOKMARKS, Icon::Bookmarks},
	{TOOLBAR_DELETEPERMANENTLY, Icon::DeletePermanently}
};

MainToolbar *MainToolbar::Create(HWND parent, HINSTANCE instance, IExplorerplusplus *pexpp,
	Navigation *navigation, std::shared_ptr<Config> config)
{
	return new MainToolbar(parent, instance, pexpp, navigation, config);
}

MainToolbar::MainToolbar(HWND parent, HINSTANCE instance, IExplorerplusplus *pexpp,
	Navigation *navigation, std::shared_ptr<Config> config) :
	CBaseWindow(CreateMainToolbar(parent)),
	m_instance(instance),
	m_pexpp(pexpp),
	m_navigation(navigation),
	m_config(config)
{
	Initialize(parent);
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
	SendMessage(m_hwnd, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

	UINT dpi = m_dpiCompat.GetDpiForWindow(m_hwnd);

	int dpiScaledSizeSmall = MulDiv(TOOLBAR_IMAGE_SIZE_SMALL, dpi, USER_DEFAULT_SCREEN_DPI);
	int dpiScaledSizeLarge = MulDiv(TOOLBAR_IMAGE_SIZE_LARGE, dpi, USER_DEFAULT_SCREEN_DPI);

	m_imageListSmall.reset(ImageList_Create(dpiScaledSizeSmall, dpiScaledSizeSmall,
		ILC_COLOR32 | ILC_MASK, 0, SIZEOF_ARRAY(TOOLBAR_BUTTON_SET)));
	m_imageListLarge.reset(ImageList_Create(dpiScaledSizeLarge, dpiScaledSizeLarge,
		ILC_COLOR32 | ILC_MASK, 0, SIZEOF_ARRAY(TOOLBAR_BUTTON_SET)));

	m_toolbarImageMapSmall = SetUpToolbarImageList(m_imageListSmall.get(), TOOLBAR_IMAGE_SIZE_SMALL, dpi);
	m_toolbarImageMapLarge = SetUpToolbarImageList(m_imageListLarge.get(), TOOLBAR_IMAGE_SIZE_LARGE, dpi);

	SetTooolbarImageList();
	SetInitialToolbarButtons();

	AddStringsToToolbar();
	AddButtonsToToolbar();

	if (m_config->showFolders)
	{
		SendMessage(m_hwnd, TB_CHECKBUTTON, TOOLBAR_FOLDERS, TRUE);
	}

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

	m_pexpp->AddTabsInitializedObserver([this] {
		m_connections.push_back(m_pexpp->GetTabContainer()->tabSelectedSignal.AddObserver(boost::bind(&MainToolbar::OnTabSelected, this, _1)));
	});

	m_connections.push_back(m_navigation->navigationCompletedSignal.AddObserver(boost::bind(&MainToolbar::OnNavigationCompleted, this, _1)));
}

void MainToolbar::SetTooolbarImageList()
{
	HIMAGELIST himl;

	if (m_config->useLargeToolbarIcons)
	{
		himl = m_imageListLarge.get();
	}
	else
	{
		himl = m_imageListSmall.get();
	}

	int cx;
	int cy;
	ImageList_GetIconSize(himl, &cx, &cy);

	SendMessage(m_hwnd, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(himl));
	SendMessage(m_hwnd, TB_SETBUTTONSIZE, 0, MAKELPARAM(cx, cy));
}

std::unordered_map<int, int> MainToolbar::SetUpToolbarImageList(HIMAGELIST imageList, int iconSize, UINT dpi)
{
	std::unordered_map<int, int> imageListMappings;

	for (const auto &mapping : TOOLBAR_BUTTON_ICON_MAPPINGS)
	{
		wil::unique_hbitmap bitmap = IconResourceLoader::LoadBitmapFromPNGForDpi(mapping.second, iconSize, iconSize, dpi);

		int imagePosition = ImageList_Add(imageList, bitmap.get(), nullptr);

		if (imagePosition == -1)
		{
			continue;
		}

		imageListMappings.insert({ mapping.first, imagePosition });
	}

	return imageListMappings;
}

MainToolbar::~MainToolbar()
{
	RemoveWindowSubclass(GetParent(m_hwnd), ParentWndProcStub, PARENT_SUBCLASS_ID);
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

			case TBN_INITCUSTOMIZE:
				return TBNRF_HIDEHELP;
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

		int imagePosition;

		if (m_config->useLargeToolbarIcons)
		{
			imagePosition = m_toolbarImageMapLarge.at(iButtonId);
		}
		else
		{
			imagePosition = m_toolbarImageMapSmall.at(iButtonId);
		}

		int stringIndex = itr->second;

		tbButton.iBitmap = imagePosition;
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

BYTE MainToolbar::LookupToolbarButtonExtraStyles(int iButtonID) const
{
	switch (iButtonID)
	{
	case TOOLBAR_BACK:
		return BTNS_DROPDOWN;
		break;

	case TOOLBAR_FORWARD:
		return BTNS_DROPDOWN;
		break;

	case TOOLBAR_FOLDERS:
		return BTNS_SHOWTEXT | BTNS_CHECK;
		break;

	case TOOLBAR_VIEWS:
		return BTNS_DROPDOWN;
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
	SetTooolbarImageList();
	UpdateToolbarButtonImageIndexes();
	SendMessage(m_hwnd, TB_AUTOSIZE, 0, 0);
}

void MainToolbar::UpdateToolbarButtonImageIndexes()
{
	int numButtons = static_cast<int>(SendMessage(m_hwnd, TB_BUTTONCOUNT, 0, 0));

	for (int i = 0; i < numButtons; i++)
	{
		TBBUTTON tbButton;
		BOOL res = static_cast<BOOL>(SendMessage(m_hwnd, TB_GETBUTTON, i, reinterpret_cast<LPARAM>(&tbButton)));

		if (!res)
		{
			continue;
		}

		if (tbButton.idCommand == 0)
		{
			// Separator.
			continue;
		}

		int imagePosition;

		if (m_config->useLargeToolbarIcons)
		{
			imagePosition = m_toolbarImageMapLarge.at(tbButton.idCommand);
		}
		else
		{
			imagePosition = m_toolbarImageMapSmall.at(tbButton.idCommand);
		}

		SendMessage(m_hwnd, TB_CHANGEBITMAP, 0, imagePosition);
	}
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
			m_navigation->BrowseFolderInCurrentTab(pidl, SBSP_ABSOLUTE | SBSP_WRITENOHISTORY);

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
			m_navigation->BrowseFolderInCurrentTab(pidl, SBSP_ABSOLUTE | SBSP_WRITENOHISTORY);

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
	ViewMode viewMode = m_pexpp->GetActiveShellBrowser()->GetViewMode();

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

	SendMessage(m_hwnd, TB_ENABLEBUTTON, TOOLBAR_NEWFOLDER, m_pexpp->CanCreate());
}

void MainToolbar::OnTabSelected(const Tab &tab)
{
	UNREFERENCED_PARAMETER(tab);

	UpdateToolbarButtonStates();
}

void MainToolbar::OnNavigationCompleted(const Tab &tab)
{
	if (m_pexpp->GetTabContainer()->IsTabSelected(tab))
	{
		UpdateToolbarButtonStates();
	}
}