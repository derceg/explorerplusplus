// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainToolbar.h"
#include "Config.h"
#include "DefaultToolbarButtons.h"
#include "Icon.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ViewModes.h"
#include "TabContainer.h"
#include "../Helper/Controls.h"
#include "../Helper/Macros.h"
#include "../Helper/XMLSettings.h"
#include <boost/bimap.hpp>
#include <gdiplus.h>

const int TOOLBAR_IMAGE_SIZE_SMALL = 16;
const int TOOLBAR_IMAGE_SIZE_LARGE = 24;

struct ToolbarButtonHash
{
	template <typename T>
	std::size_t operator()(T t) const
	{
		return t._to_integral();
	}
};

const std::unordered_map<ToolbarButton, Icon, ToolbarButtonHash> TOOLBAR_BUTTON_ICON_MAPPINGS = {
	{ToolbarButton::TOOLBAR_BACK, Icon::Back},
	{ToolbarButton::TOOLBAR_FORWARD, Icon::Forward},
	{ToolbarButton::TOOLBAR_UP, Icon::Up},
	{ToolbarButton::TOOLBAR_FOLDERS, Icon::FolderTree},
	{ToolbarButton::TOOLBAR_COPY_TO, Icon::CopyTo},
	{ToolbarButton::TOOLBAR_MOVE_TO, Icon::MoveTo},
	{ToolbarButton::TOOLBAR_NEW_FOLDER, Icon::NewFolder},
	{ToolbarButton::TOOLBAR_COPY, Icon::Copy},
	{ToolbarButton::TOOLBAR_CUT, Icon::Cut},
	{ToolbarButton::TOOLBAR_PASTE, Icon::Paste},
	{ToolbarButton::TOOLBAR_DELETE, Icon::Delete},
	{ToolbarButton::TOOLBAR_VIEWS, Icon::Views},
	{ToolbarButton::TOOLBAR_SEARCH, Icon::Search},
	{ToolbarButton::TOOLBAR_PROPERTIES, Icon::Properties},
	{ToolbarButton::TOOLBAR_REFRESH, Icon::Refresh},
	{ToolbarButton::TOOLBAR_ADD_BOOKMARK, Icon::AddBookmark},
	{ToolbarButton::TOOLBAR_NEW_TAB, Icon::NewTab},
	{ToolbarButton::TOOLBAR_OPEN_COMMAND_PROMPT, Icon::CommandLine},
	{ToolbarButton::TOOLBAR_ORGANIZE_BOOKMARKS, Icon::Bookmarks},
	{ToolbarButton::TOOLBAR_DELETE_PERMANENTLY, Icon::DeletePermanently},
	{ToolbarButton::TOOLBAR_SPLIT_FILE, Icon::SplitFiles},
	{ToolbarButton::TOOLBAR_MERGE_FILES, Icon::MergeFiles}
};

template <typename L, typename R>
boost::bimap<L, R>
MakeBimap(std::initializer_list<typename boost::bimap<L, R>::value_type> list)
{
	return boost::bimap<L, R>(list.begin(), list.end());
}

#pragma warning(push)
#pragma warning(disable:4996) //warning STL4010: Various members of std::allocator are deprecated in C++17.

// Ideally, toolbar button IDs would be saved in the XML config file, rather
// than button strings, but that's not especially easy to change now.
const boost::bimap<ToolbarButton, std::wstring> TOOLBAR_BUTTON_XML_NAME_MAPPINGS = MakeBimap<ToolbarButton, std::wstring>({
	{ToolbarButton::TOOLBAR_BACK, L"Back"},
	{ToolbarButton::TOOLBAR_FORWARD, L"Forward"},
	{ToolbarButton::TOOLBAR_UP, L"Up"},
	{ToolbarButton::TOOLBAR_FOLDERS, L"Folders"},
	{ToolbarButton::TOOLBAR_COPY_TO, L"Copy To"},
	{ToolbarButton::TOOLBAR_MOVE_TO, L"Move To"},
	{ToolbarButton::TOOLBAR_NEW_FOLDER, L"New Folder"},
	{ToolbarButton::TOOLBAR_COPY, L"Copy"},
	{ToolbarButton::TOOLBAR_CUT, L"Cut"},
	{ToolbarButton::TOOLBAR_PASTE, L"Paste"},
	{ToolbarButton::TOOLBAR_DELETE, L"Delete"},
	{ToolbarButton::TOOLBAR_VIEWS, L"Views"},
	{ToolbarButton::TOOLBAR_SEARCH, L"Search"},
	{ToolbarButton::TOOLBAR_PROPERTIES, L"Properties"},
	{ToolbarButton::TOOLBAR_REFRESH, L"Refresh"},
	{ToolbarButton::TOOLBAR_ADD_BOOKMARK, L"Bookmark the current tab"},
	{ToolbarButton::TOOLBAR_NEW_TAB, L"Create a new tab"},
	{ToolbarButton::TOOLBAR_OPEN_COMMAND_PROMPT, L"Open Command Prompt"},
	{ToolbarButton::TOOLBAR_ORGANIZE_BOOKMARKS, L"Organize Bookmarks"},
	{ToolbarButton::TOOLBAR_DELETE_PERMANENTLY, L"Delete Permanently"},
	{ToolbarButton::TOOLBAR_SPLIT_FILE, L"Split File"},
	{ToolbarButton::TOOLBAR_MERGE_FILES, L"Merge Files"},

	{ToolbarButton::TOOLBAR_SEPARATOR, L"Separator"}
});

#pragma warning(pop)

MainToolbar *MainToolbar::Create(HWND parent, HINSTANCE instance, IExplorerplusplus *pexpp,
	Navigation *navigation, std::shared_ptr<Config> config)
{
	return new MainToolbar(parent, instance, pexpp, navigation, config);
}

MainToolbar::MainToolbar(HWND parent, HINSTANCE instance, IExplorerplusplus *pexpp,
	Navigation *navigation, std::shared_ptr<Config> config) :
	CBaseWindow(CreateMainToolbar(parent)),
	m_persistentSettings(&MainToolbarPersistentSettings::GetInstance()),
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
	// Ideally, this constraint would be checked at compile-time, but the size
	// of TOOLBAR_BUTTON_ICON_MAPPINGS isn't known at compile-time. Note that
	// the ToolbarButton enum contains one additional item - for the separator.
	assert(TOOLBAR_BUTTON_ICON_MAPPINGS.size() == (ToolbarButton::_size() - 1));

	SendMessage(m_hwnd, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

	UINT dpi = m_dpiCompat.GetDpiForWindow(m_hwnd);

	int dpiScaledSizeSmall = MulDiv(TOOLBAR_IMAGE_SIZE_SMALL, dpi, USER_DEFAULT_SCREEN_DPI);
	int dpiScaledSizeLarge = MulDiv(TOOLBAR_IMAGE_SIZE_LARGE, dpi, USER_DEFAULT_SCREEN_DPI);

	m_imageListSmall.reset(ImageList_Create(dpiScaledSizeSmall, dpiScaledSizeSmall,
		ILC_COLOR32 | ILC_MASK, 0, static_cast<int>(ToolbarButton::_size() - 1)));
	m_imageListLarge.reset(ImageList_Create(dpiScaledSizeLarge, dpiScaledSizeLarge,
		ILC_COLOR32 | ILC_MASK, 0, static_cast<int>(ToolbarButton::_size() - 1)));

	m_toolbarImageMapSmall = SetUpToolbarImageList(m_imageListSmall.get(), m_pexpp->GetIconResourceLoader(), TOOLBAR_IMAGE_SIZE_SMALL, dpi);
	m_toolbarImageMapLarge = SetUpToolbarImageList(m_imageListLarge.get(), m_pexpp->GetIconResourceLoader(), TOOLBAR_IMAGE_SIZE_LARGE, dpi);

	SetTooolbarImageList();
	AddStringsToToolbar();
	AddButtonsToToolbar(m_persistentSettings->m_toolbarButtons);

	if (m_config->showFolders)
	{
		SendMessage(m_hwnd, TB_CHECKBUTTON, ToolbarButton::TOOLBAR_FOLDERS, TRUE);
	}

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

std::unordered_map<int, int> MainToolbar::SetUpToolbarImageList(HIMAGELIST imageList,
	IconResourceLoader *iconResourceLoader, int iconSize, UINT dpi)
{
	std::unordered_map<int, int> imageListMappings;

	for (const auto &mapping : TOOLBAR_BUTTON_ICON_MAPPINGS)
	{
		wil::unique_hbitmap bitmap = iconResourceLoader->LoadBitmapFromPNGForDpi(mapping.second, iconSize, iconSize, dpi);

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

			case TBN_TOOLBARCHANGE:
				OnTBChange();
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

void MainToolbar::AddButtonsToToolbar(const std::vector<ToolbarButton> &buttons)
{
	for (auto button : buttons)
	{
		AddButtonToToolbar(button);
	}
}

void MainToolbar::AddButtonToToolbar(ToolbarButton button)
{
	TBBUTTON tbButton = GetToolbarButtonDetails(button);
	SendMessage(m_hwnd, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&tbButton));
}

TBBUTTON MainToolbar::GetToolbarButtonDetails(ToolbarButton button) const
{
	TBBUTTON tbButton;

	ZeroMemory(&tbButton, sizeof(tbButton));

	if (button == +ToolbarButton::TOOLBAR_SEPARATOR)
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

		auto stringIndex = m_toolbarStringMap.at(button);

		int imagePosition;

		if (m_config->useLargeToolbarIcons)
		{
			imagePosition = m_toolbarImageMapLarge.at(button);
		}
		else
		{
			imagePosition = m_toolbarImageMapSmall.at(button);
		}

		tbButton.iBitmap = imagePosition;
		tbButton.idCommand = button;
		tbButton.fsState = TBSTATE_ENABLED;
		tbButton.fsStyle = StandardStyle | LookupToolbarButtonExtraStyles(button);
		tbButton.dwData = 0;
		tbButton.iString = stringIndex;
	}

	return tbButton;
}

void MainToolbar::AddStringsToToolbar()
{
	for (auto button : ToolbarButton::_values())
	{
		if (button == +ToolbarButton::TOOLBAR_SEPARATOR)
		{
			continue;
		}

		AddStringToToolbar(button);
	}
}

void MainToolbar::AddStringToToolbar(ToolbarButton button)
{
	TCHAR szText[64];

	/* The string must be double NULL-terminated. */
	GetToolbarButtonText(button, szText, SIZEOF_ARRAY(szText));
	szText[lstrlen(szText) + 1] = '\0';

	int index = static_cast<int>(SendMessage(m_hwnd, TB_ADDSTRING, NULL, reinterpret_cast<LPARAM>(szText)));

	m_toolbarStringMap.insert(std::make_pair(button, index));
}

void MainToolbar::GetToolbarButtonText(ToolbarButton button, TCHAR *szText, int bufSize) const
{
	int res = LoadString(m_instance, LookupToolbarButtonTextID(button), szText, bufSize);
	assert(res != 0);

	/* It doesn't really make sense to return this. If the string isn't in the
	string table, there's a bug somewhere in the program. */
	UNUSED(res);
}

BYTE MainToolbar::LookupToolbarButtonExtraStyles(ToolbarButton button) const
{
	switch (button)
	{
	case ToolbarButton::TOOLBAR_BACK:
		return BTNS_DROPDOWN;
		break;

	case ToolbarButton::TOOLBAR_FORWARD:
		return BTNS_DROPDOWN;
		break;

	case ToolbarButton::TOOLBAR_FOLDERS:
		return BTNS_SHOWTEXT | BTNS_CHECK;
		break;

	case ToolbarButton::TOOLBAR_VIEWS:
		return BTNS_DROPDOWN;
		break;
	}

	return 0;
}

int MainToolbar::LookupToolbarButtonTextID(ToolbarButton button) const
{
	switch (button)
	{
	case ToolbarButton::TOOLBAR_SEPARATOR:
		return IDS_SEPARATOR;
		break;

	case ToolbarButton::TOOLBAR_BACK:
		return IDS_TOOLBAR_BACK;
		break;

	case ToolbarButton::TOOLBAR_FORWARD:
		return IDS_TOOLBAR_FORWARD;
		break;

	case ToolbarButton::TOOLBAR_UP:
		return IDS_TOOLBAR_UP;
		break;

	case ToolbarButton::TOOLBAR_FOLDERS:
		return IDS_TOOLBAR_FOLDERS;
		break;

	case ToolbarButton::TOOLBAR_COPY_TO:
		return IDS_TOOLBAR_COPYTO;
		break;

	case ToolbarButton::TOOLBAR_MOVE_TO:
		return IDS_TOOLBAR_MOVETO;
		break;

	case ToolbarButton::TOOLBAR_NEW_FOLDER:
		return IDS_TOOLBAR_NEWFOLDER;
		break;

	case ToolbarButton::TOOLBAR_COPY:
		return IDS_TOOLBAR_COPY;
		break;

	case ToolbarButton::TOOLBAR_CUT:
		return IDS_TOOLBAR_CUT;
		break;

	case ToolbarButton::TOOLBAR_PASTE:
		return IDS_TOOLBAR_PASTE;
		break;

	case ToolbarButton::TOOLBAR_DELETE:
		return IDS_TOOLBAR_DELETE;
		break;

	case ToolbarButton::TOOLBAR_DELETE_PERMANENTLY:
		return IDS_TOOLBAR_DELETEPERMANENTLY;
		break;

	case ToolbarButton::TOOLBAR_VIEWS:
		return IDS_TOOLBAR_VIEWS;
		break;

	case ToolbarButton::TOOLBAR_SEARCH:
		return IDS_TOOLBAR_SEARCH;
		break;

	case ToolbarButton::TOOLBAR_PROPERTIES:
		return IDS_TOOLBAR_PROPERTIES;
		break;

	case ToolbarButton::TOOLBAR_REFRESH:
		return IDS_TOOLBAR_REFRESH;
		break;

	case ToolbarButton::TOOLBAR_ADD_BOOKMARK:
		return IDS_TOOLBAR_ADDBOOKMARK;
		break;

	case ToolbarButton::TOOLBAR_ORGANIZE_BOOKMARKS:
		return IDS_TOOLBAR_MANAGEBOOKMARKS;
		break;

	case ToolbarButton::TOOLBAR_NEW_TAB:
		return IDS_TOOLBAR_NEWTAB;
		break;

	case ToolbarButton::TOOLBAR_OPEN_COMMAND_PROMPT:
		return IDS_TOOLBAR_OPENCOMMANDPROMPT;
		break;

	case ToolbarButton::TOOLBAR_SPLIT_FILE:
		return IDS_TOOLBAR_SPLIT_FILE;
		break;

	case ToolbarButton::TOOLBAR_MERGE_FILES:
		return IDS_TOOLBAR_MERGE_FILES;
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

	if ((pnmtb->iItem >= 0) && (pnmtb->iItem < (ToolbarButton::_size() - 1)))
	{
		// Note that the separator (which is the first item in the enumeration)
		// is skipped.
		ToolbarButton button = ToolbarButton::_values()[pnmtb->iItem + 1];
		pnmtb->tbButton = GetToolbarButtonDetails(button);

		TCHAR szText[64];
		GetToolbarButtonText(button, szText, SIZEOF_ARRAY(szText));
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
	int numButtons = static_cast<int>(SendMessage(m_hwnd, TB_BUTTONCOUNT, 0, 0));

	for (int i = numButtons - 1; i >= 0; i--)
	{
		SendMessage(m_hwnd, TB_DELETEBUTTON, i, 0);
	}

	m_persistentSettings->m_toolbarButtons = { DEFAULT_TOOLBAR_BUTTONS, std::end(DEFAULT_TOOLBAR_BUTTONS) };

	AddButtonsToToolbar(m_persistentSettings->m_toolbarButtons);
	UpdateToolbarButtonStates();
}

void MainToolbar::OnTBChange()
{
	std::vector<ToolbarButton> toolbarButtons;
	int numButtons = static_cast<int>(SendMessage(m_hwnd, TB_BUTTONCOUNT, 0, 0));

	for (int i = 0; i < numButtons; i++)
	{
		TBBUTTON tbButton;
		BOOL res = static_cast<BOOL>(SendMessage(m_hwnd, TB_GETBUTTON, i, reinterpret_cast<LPARAM>(&tbButton)));

		if (!res)
		{
			continue;
		}

		int id;

		if (tbButton.idCommand == 0)
		{
			id = ToolbarButton::TOOLBAR_SEPARATOR;
		}
		else
		{
			id = tbButton.idCommand;
		}

		toolbarButtons.push_back(ToolbarButton::_from_integral(id));
	}

	m_persistentSettings->m_toolbarButtons = toolbarButtons;
}

void MainToolbar::OnTBGetInfoTip(LPARAM lParam)
{
	NMTBGETINFOTIP *ptbgit = reinterpret_cast<NMTBGETINFOTIP *>(lParam);

	StringCchCopy(ptbgit->pszText, ptbgit->cchTextMax, EMPTY_STRING);

	if (ptbgit->iItem == ToolbarButton::TOOLBAR_BACK)
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
	else if (ptbgit->iItem == ToolbarButton::TOOLBAR_FORWARD)
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

	if (nmTB->iItem == ToolbarButton::TOOLBAR_BACK)
	{
		hr = m_pexpp->GetActiveShellBrowser()->CreateHistoryPopup(m_hwnd, &pidl, &ptOrigin, TRUE);

		if (SUCCEEDED(hr))
		{
			m_navigation->BrowseFolderInCurrentTab(pidl, SBSP_ABSOLUTE | SBSP_WRITENOHISTORY);

			CoTaskMemFree(pidl);
		}

		return TBDDRET_DEFAULT;
	}
	else if (nmTB->iItem == ToolbarButton::TOOLBAR_FORWARD)
	{
		SendMessage(m_hwnd, TB_GETRECT, (WPARAM)ToolbarButton::TOOLBAR_BACK, (LPARAM)&rc);

		ptOrigin.x += rc.right;

		hr = m_pexpp->GetActiveShellBrowser()->CreateHistoryPopup(m_hwnd, &pidl, &ptOrigin, FALSE);

		if (SUCCEEDED(hr))
		{
			m_navigation->BrowseFolderInCurrentTab(pidl, SBSP_ABSOLUTE | SBSP_WRITENOHISTORY);

			CoTaskMemFree(pidl);
		}

		return TBDDRET_DEFAULT;
	}
	else if (nmTB->iItem == ToolbarButton::TOOLBAR_VIEWS)
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

	SendMessage(m_hwnd, TB_GETRECT, (WPARAM)ToolbarButton::TOOLBAR_VIEWS, (LPARAM)&rcButton);

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
	SendMessage(m_hwnd, TB_ENABLEBUTTON, ToolbarButton::TOOLBAR_UP, m_pexpp->GetActiveShellBrowser()->CanBrowseUp());

	SendMessage(m_hwnd, TB_ENABLEBUTTON, ToolbarButton::TOOLBAR_BACK, m_pexpp->GetActiveShellBrowser()->CanBrowseBack());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, ToolbarButton::TOOLBAR_FORWARD, m_pexpp->GetActiveShellBrowser()->CanBrowseForward());

	BOOL bVirtualFolder = m_pexpp->GetActiveShellBrowser()->InVirtualFolder();

	SendMessage(m_hwnd, TB_ENABLEBUTTON, (WPARAM)ToolbarButton::TOOLBAR_COPY_TO, m_pexpp->CanCopy() && GetFocus() != m_pexpp->GetTreeView());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, (WPARAM)ToolbarButton::TOOLBAR_MOVE_TO, m_pexpp->CanCut() && GetFocus() != m_pexpp->GetTreeView());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, (WPARAM)ToolbarButton::TOOLBAR_COPY, m_pexpp->CanCopy());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, (WPARAM)ToolbarButton::TOOLBAR_CUT, m_pexpp->CanCut());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, (WPARAM)ToolbarButton::TOOLBAR_PASTE, m_pexpp->CanPaste());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, (WPARAM)ToolbarButton::TOOLBAR_PROPERTIES, m_pexpp->CanShowFileProperties());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, (WPARAM)ToolbarButton::TOOLBAR_DELETE, m_pexpp->CanDelete());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, (WPARAM)ToolbarButton::TOOLBAR_DELETE_PERMANENTLY, m_pexpp->CanDelete());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, (WPARAM)ToolbarButton::TOOLBAR_SPLIT_FILE, m_pexpp->GetActiveShellBrowser()->GetNumSelectedFiles() == 1);
	SendMessage(m_hwnd, TB_ENABLEBUTTON, (WPARAM)ToolbarButton::TOOLBAR_MERGE_FILES, m_pexpp->GetActiveShellBrowser()->GetNumSelectedFiles() > 1);
	SendMessage(m_hwnd, TB_ENABLEBUTTON, (WPARAM)ToolbarButton::TOOLBAR_OPEN_COMMAND_PROMPT, !bVirtualFolder);
	SendMessage(m_hwnd, TB_ENABLEBUTTON, ToolbarButton::TOOLBAR_NEW_FOLDER, m_pexpp->CanCreate());
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

MainToolbarPersistentSettings::MainToolbarPersistentSettings() :
	m_toolbarButtons(DEFAULT_TOOLBAR_BUTTONS, std::end(DEFAULT_TOOLBAR_BUTTONS))
{
	assert(TOOLBAR_BUTTON_XML_NAME_MAPPINGS.size() == ToolbarButton::_size());
}

MainToolbarPersistentSettings &MainToolbarPersistentSettings::GetInstance()
{
	static MainToolbarPersistentSettings persistentSettings;
	return persistentSettings;
}

void MainToolbarPersistentSettings::LoadXMLSettings(IXMLDOMNode *pNode)
{
	IXMLDOMNode *pChildNode = NULL;
	IXMLDOMNamedNodeMap *am = NULL;
	BSTR bstrValue;

	std::vector<ToolbarButton> toolbarButtons;

	pNode->get_attributes(&am);

	long lChildNodes;
	long j = 0;

	/* Retrieve the total number of attributes
	attached to this node. */
	am->get_length(&lChildNodes);

	for (j = 1; j < lChildNodes; j++)
	{
		am->get_item(j, &pChildNode);

		/* Element value. */
		pChildNode->get_text(&bstrValue);

		auto itr = TOOLBAR_BUTTON_XML_NAME_MAPPINGS.right.find(bstrValue);

		if (itr == TOOLBAR_BUTTON_XML_NAME_MAPPINGS.right.end())
		{
			continue;
		}

		toolbarButtons.push_back(itr->second);
	}

	m_toolbarButtons = toolbarButtons;
}

void MainToolbarPersistentSettings::SaveXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pe)
{
	int index = 0;

	for (auto button : m_toolbarButtons)
	{
		TCHAR szButtonAttributeName[32];
		StringCchPrintf(szButtonAttributeName, SIZEOF_ARRAY(szButtonAttributeName), _T("Button%d"), index);

		std::wstring buttonName = TOOLBAR_BUTTON_XML_NAME_MAPPINGS.left.at(button);

		NXMLSettings::AddAttributeToNode(pXMLDom, pe, szButtonAttributeName, buttonName.c_str());

		index++;
	}
}