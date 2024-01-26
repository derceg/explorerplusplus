// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainToolbar.h"
#include "Config.h"
#include "DefaultToolbarButtons.h"
#include "Icon.h"
#include "MainResource.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowser/ViewModes.h"
#include "TabContainer.h"
#include "../Helper/Controls.h"
#include "../Helper/ImageHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/XMLSettings.h"
#include <boost/bimap.hpp>

// Enable C4062: enumerator 'identifier' in switch of enum 'enumeration' is not handled
#pragma warning(default:4062)

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
	{ToolbarButton::Back, Icon::Back},
	{ToolbarButton::Forward, Icon::Forward},
	{ToolbarButton::Up, Icon::Up},
	{ToolbarButton::Folders, Icon::FolderTree},
	{ToolbarButton::CopyTo, Icon::CopyTo},
	{ToolbarButton::MoveTo, Icon::MoveTo},
	{ToolbarButton::NewFolder, Icon::NewFolder},
	{ToolbarButton::Copy, Icon::Copy},
	{ToolbarButton::Cut, Icon::Cut},
	{ToolbarButton::Paste, Icon::Paste},
	{ToolbarButton::Delete, Icon::Delete},
	{ToolbarButton::Views, Icon::Views},
	{ToolbarButton::Search, Icon::Search},
	{ToolbarButton::Properties, Icon::Properties},
	{ToolbarButton::Refresh, Icon::Refresh},
	{ToolbarButton::AddBookmark, Icon::AddBookmark},
	{ToolbarButton::NewTab, Icon::NewTab},
	{ToolbarButton::OpenCommandPrompt, Icon::CommandLine},
	{ToolbarButton::Bookmarks, Icon::Bookmarks},
	{ToolbarButton::DeletePermanently, Icon::DeletePermanently},
	{ToolbarButton::SplitFile, Icon::SplitFiles},
	{ToolbarButton::MergeFiles, Icon::MergeFiles},
	{ToolbarButton::CloseTab, Icon::CloseTab}
};

template <typename L, typename R>
boost::bimap<L, R>
MakeBimap(std::initializer_list<typename boost::bimap<L, R>::value_type> list)
{
	return boost::bimap<L, R>(list.begin(), list.end());
}

#pragma warning(push)
// warning STL4010: Various members of std::allocator are deprecated in C++17.
// warning C4834: discarding return value of function with 'nodiscard' attribute
#pragma warning(disable: 4996 4834)

// Ideally, toolbar button IDs would be saved in the XML config file, rather
// than button strings, but that's not especially easy to change now.
const boost::bimap<ToolbarButton, std::wstring> TOOLBAR_BUTTON_XML_NAME_MAPPINGS = MakeBimap<ToolbarButton, std::wstring>({
	{ToolbarButton::Back, L"Back"},
	{ToolbarButton::Forward, L"Forward"},
	{ToolbarButton::Up, L"Up"},
	{ToolbarButton::Folders, L"Folders"},
	{ToolbarButton::CopyTo, L"Copy To"},
	{ToolbarButton::MoveTo, L"Move To"},
	{ToolbarButton::NewFolder, L"New Folder"},
	{ToolbarButton::Copy, L"Copy"},
	{ToolbarButton::Cut, L"Cut"},
	{ToolbarButton::Paste, L"Paste"},
	{ToolbarButton::Delete, L"Delete"},
	{ToolbarButton::Views, L"Views"},
	{ToolbarButton::Search, L"Search"},
	{ToolbarButton::Properties, L"Properties"},
	{ToolbarButton::Refresh, L"Refresh"},
	{ToolbarButton::AddBookmark, L"Bookmark the current tab"},
	{ToolbarButton::NewTab, L"Create a new tab"},
	{ToolbarButton::OpenCommandPrompt, L"Open Command Prompt"},
	{ToolbarButton::Bookmarks, L"Organize Bookmarks"},
	{ToolbarButton::DeletePermanently, L"Delete Permanently"},
	{ToolbarButton::SplitFile, L"Split File"},
	{ToolbarButton::MergeFiles, L"Merge Files"},
	{ToolbarButton::CloseTab, L"Close Tab"},

	{ToolbarButton::Separator, L"Separator"}
});

#pragma warning(pop)

MainToolbar *MainToolbar::Create(HWND parent, HINSTANCE instance, IExplorerplusplus *pexpp,
	std::shared_ptr<Config> config)
{
	return new MainToolbar(parent, instance, pexpp, config);
}

MainToolbar::MainToolbar(HWND parent, HINSTANCE instance, IExplorerplusplus *pexpp,
	std::shared_ptr<Config> config) :
	BaseWindow(CreateMainToolbar(parent)),
	m_persistentSettings(&MainToolbarPersistentSettings::GetInstance()),
	m_instance(instance),
	m_pexpp(pexpp),
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

	SHGetImageList(SHIL_SYSSMALL, IID_PPV_ARGS(&m_systemImageList));

	m_defaultFolderIconBitmap = ImageHelper::ImageListIconToBitmap(m_systemImageList.get(), GetDefaultFolderIconIndex());

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
	UpdateConfigDependentButtonStates();

	m_windowSubclasses.emplace_back(parent, ParentWndProcStub, PARENT_SUBCLASS_ID,
		reinterpret_cast<DWORD_PTR>(this));

	m_pexpp->AddTabsInitializedObserver([this] {
		m_connections.push_back(m_pexpp->GetTabContainer()->tabSelectedSignal.AddObserver(
			boost::bind(&MainToolbar::OnTabSelected, this, _1)));
		m_connections.push_back(m_pexpp->GetTabContainer()->tabNavigationCompletedSignal.AddObserver(
			boost::bind(&MainToolbar::OnNavigationCompleted, this, _1)));
	});

	m_connections.push_back(
		m_pexpp->AddFocusChangeObserver(boost::bind(&MainToolbar::OnFocusChanged, this, _1)));
	m_connections.push_back(m_config->useLargeToolbarIcons.addObserver(boost::bind(&MainToolbar::OnUseLargeToolbarIconsUpdated, this, _1)));
}

void MainToolbar::SetTooolbarImageList()
{
	HIMAGELIST himl;

	if (m_config->useLargeToolbarIcons.get())
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

LRESULT CALLBACK MainToolbar::ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *mainToolbar = reinterpret_cast<MainToolbar *>(dwRefData);
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

			case TBN_QUERYDELETE:
				return OnTBQueryDelete();

			case TBN_GETBUTTONINFO:
				return OnTBGetButtonInfo(lParam);

			case TBN_RESTORE:
				return OnTBRestore();

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
				return OnTbnDropDown(reinterpret_cast<NMTOOLBAR *>(lParam));

			case TBN_INITCUSTOMIZE:
				return TBNRF_HIDEHELP;
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

	if (button == +ToolbarButton::Separator)
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
		BYTE standardStyle = BTNS_BUTTON | BTNS_AUTOSIZE;

		auto stringIndex = m_toolbarStringMap.at(button);

		int imagePosition;

		if (m_config->useLargeToolbarIcons.get())
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
		tbButton.fsStyle = standardStyle | LookupToolbarButtonExtraStyles(button);
		tbButton.dwData = 0;
		tbButton.iString = stringIndex;
	}

	return tbButton;
}

void MainToolbar::AddStringsToToolbar()
{
	for (auto button : ToolbarButton::_values())
	{
		if (button == +ToolbarButton::Separator)
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
	case ToolbarButton::Back:
		return BTNS_DROPDOWN;

	case ToolbarButton::Forward:
		return BTNS_DROPDOWN;

	case ToolbarButton::Folders:
		return BTNS_SHOWTEXT | BTNS_CHECK;

	case ToolbarButton::Views:
		return BTNS_DROPDOWN;

	default:
		return 0;
	}
}

int MainToolbar::LookupToolbarButtonTextID(ToolbarButton button) const
{
	switch (button)
	{
	case ToolbarButton::Separator:
		return IDS_SEPARATOR;

	case ToolbarButton::Back:
		return IDS_TOOLBAR_BACK;

	case ToolbarButton::Forward:
		return IDS_TOOLBAR_FORWARD;

	case ToolbarButton::Up:
		return IDS_TOOLBAR_UP;

	case ToolbarButton::Folders:
		return IDS_TOOLBAR_FOLDERS;

	case ToolbarButton::CopyTo:
		return IDS_TOOLBAR_COPYTO;

	case ToolbarButton::MoveTo:
		return IDS_TOOLBAR_MOVETO;

	case ToolbarButton::NewFolder:
		return IDS_TOOLBAR_NEWFOLDER;

	case ToolbarButton::Copy:
		return IDS_TOOLBAR_COPY;

	case ToolbarButton::Cut:
		return IDS_TOOLBAR_CUT;

	case ToolbarButton::Paste:
		return IDS_TOOLBAR_PASTE;

	case ToolbarButton::Delete:
		return IDS_TOOLBAR_DELETE;

	case ToolbarButton::DeletePermanently:
		return IDS_TOOLBAR_DELETEPERMANENTLY;

	case ToolbarButton::Views:
		return IDS_TOOLBAR_VIEWS;

	case ToolbarButton::Search:
		return IDS_TOOLBAR_SEARCH;

	case ToolbarButton::Properties:
		return IDS_TOOLBAR_PROPERTIES;

	case ToolbarButton::Refresh:
		return IDS_TOOLBAR_REFRESH;

	case ToolbarButton::AddBookmark:
		return IDS_TOOLBAR_ADDBOOKMARK;

	case ToolbarButton::Bookmarks:
		return IDS_TOOLBAR_MANAGEBOOKMARKS;

	case ToolbarButton::NewTab:
		return IDS_TOOLBAR_NEWTAB;

	case ToolbarButton::OpenCommandPrompt:
		return IDS_TOOLBAR_OPENCOMMANDPROMPT;

	case ToolbarButton::SplitFile:
		return IDS_TOOLBAR_SPLIT_FILE;

	case ToolbarButton::MergeFiles:
		return IDS_TOOLBAR_MERGE_FILES;

	case ToolbarButton::CloseTab:
		return IDS_TOOLBAR_CLOSE_TAB;
	}

	return 0;
}

void MainToolbar::OnUseLargeToolbarIconsUpdated(BOOL newValue)
{
	UNREFERENCED_PARAMETER(newValue);

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

		if (m_config->useLargeToolbarIcons.get())
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
	auto *pnmtb = reinterpret_cast<NMTOOLBAR *>(lParam);

	if ((pnmtb->iItem >= 0) && (static_cast<std::size_t>(pnmtb->iItem) < (ToolbarButton::_size() - 1)))
	{
		// Note that the separator (which is the first item in the enumeration)
		// is skipped.
		assert(ToolbarButton::_values()[0] == +ToolbarButton::Separator);
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
	UpdateConfigDependentButtonStates();
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
			id = ToolbarButton::Separator;
		}
		else
		{
			id = tbButton.idCommand;
		}

		toolbarButtons.push_back(ToolbarButton::_from_integral(id));
	}

	m_persistentSettings->m_toolbarButtons = toolbarButtons;

	UpdateConfigDependentButtonStates();
}

void MainToolbar::OnTBGetInfoTip(LPARAM lParam)
{
	auto *ptbgit = reinterpret_cast<NMTBGETINFOTIP *>(lParam);

	StringCchCopy(ptbgit->pszText, ptbgit->cchTextMax, EMPTY_STRING);

	const Tab &tab = m_pexpp->GetTabContainer()->GetSelectedTab();

	if (ptbgit->iItem == ToolbarButton::Back)
	{
		auto entry = tab.GetShellBrowser()->GetNavigationController()->GetEntry(-1);

		if (entry)
		{
			TCHAR szInfoTip[1024];
			TCHAR szTemp[64];
			LoadString(m_instance, IDS_MAIN_TOOLBAR_BACK, szTemp, SIZEOF_ARRAY(szTemp));
			StringCchPrintf(szInfoTip, SIZEOF_ARRAY(szInfoTip), szTemp, entry->GetDisplayName().c_str());

			StringCchCopy(ptbgit->pszText, ptbgit->cchTextMax, szInfoTip);
		}
	}
	else if (ptbgit->iItem == ToolbarButton::Forward)
	{
		auto entry = tab.GetShellBrowser()->GetNavigationController()->GetEntry(1);

		if (entry)
		{
			TCHAR szInfoTip[1024];
			TCHAR szTemp[64];
			LoadString(m_instance, IDS_MAIN_TOOLBAR_FORWARD, szTemp, SIZEOF_ARRAY(szTemp));
			StringCchPrintf(szInfoTip, SIZEOF_ARRAY(szInfoTip), szTemp, entry->GetDisplayName().c_str());

			StringCchCopy(ptbgit->pszText, ptbgit->cchTextMax, szInfoTip);
		}
	}
}

LRESULT MainToolbar::OnTbnDropDown(const NMTOOLBAR *nmtb)
{
	RECT toolbarRect;
	GetWindowRect(m_hwnd, &toolbarRect);

	POINT ptOrigin;
	ptOrigin.x = toolbarRect.left;
	ptOrigin.y = toolbarRect.bottom - 4;

	if (nmtb->iItem == ToolbarButton::Back)
	{
		ShowHistoryMenu(HistoryType::Back, ptOrigin);

		return TBDDRET_DEFAULT;
	}
	else if (nmtb->iItem == ToolbarButton::Forward)
	{
		RECT backButtonRect;
		SendMessage(m_hwnd, TB_GETRECT, ToolbarButton::Back, reinterpret_cast<LPARAM>(&backButtonRect));

		ptOrigin.x += backButtonRect.right;

		ShowHistoryMenu(HistoryType::Forward, ptOrigin);

		return TBDDRET_DEFAULT;
	}
	else if (nmtb->iItem == ToolbarButton::Views)
	{
		ShowToolbarViewsDropdown();

		return TBDDRET_DEFAULT;
	}

	return TBDDRET_NODEFAULT;
}

void MainToolbar::ShowHistoryMenu(HistoryType historyType, const POINT &pt)
{
	std::vector<HistoryEntry *> history;

	const Tab &tab = m_pexpp->GetTabContainer()->GetSelectedTab();

	if (historyType == HistoryType::Back)
	{
		history = tab.GetShellBrowser()->GetNavigationController()->GetBackHistory();
	}
	else
	{
		history = tab.GetShellBrowser()->GetNavigationController()->GetForwardHistory();
	}

	if (history.empty())
	{
		return;
	}

	wil::unique_hmenu menu(CreatePopupMenu());
	std::vector<wil::unique_hbitmap> menuImages;
	int numInserted = 0;

	for (auto &entry : history)
	{
		std::wstring displayName = entry->GetDisplayName();

		MENUITEMINFO mii;
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_ID | MIIM_STRING;
		mii.wID = numInserted + 1;
		mii.dwTypeData = displayName.data();

		HBITMAP bitmap = nullptr;
		auto iconIndex = entry->GetSystemIconIndex();

		if (iconIndex)
		{
			wil::unique_hbitmap iconBitmap = ImageHelper::ImageListIconToBitmap(m_systemImageList.get(), *iconIndex);

			if (iconBitmap)
			{
				bitmap = iconBitmap.get();
				menuImages.push_back(std::move(iconBitmap));
			}
		}
		else
		{
			bitmap = m_defaultFolderIconBitmap.get();
		}

		if (bitmap)
		{
			mii.fMask |= MIIM_BITMAP;
			mii.hbmpItem = bitmap;
		}

		InsertMenuItem(menu.get(), numInserted, TRUE, &mii);

		numInserted++;
	}

	int cmd = TrackPopupMenu(menu.get(), TPM_LEFTALIGN | TPM_VERTICAL | TPM_RETURNCMD,
		pt.x, pt.y, 0, m_hwnd, nullptr);

	if (cmd == 0)
	{
		return;
	}

	if (historyType == HistoryType::Back)
	{
		cmd = -cmd;
	}

	Tab &selectedTab = m_pexpp->GetTabContainer()->GetSelectedTab();
	selectedTab.GetShellBrowser()->GetNavigationController()->GoToOffset(cmd);
}

void MainToolbar::ShowToolbarViewsDropdown()
{
	POINT	ptOrigin;
	RECT	rcButton;

	SendMessage(m_hwnd, TB_GETRECT, (WPARAM)ToolbarButton::Views, (LPARAM)&rcButton);

	ptOrigin.x = rcButton.left;
	ptOrigin.y = rcButton.bottom;

	ClientToScreen(m_hwnd, &ptOrigin);

	CreateViewsMenu(&ptOrigin);
}

void MainToolbar::CreateViewsMenu(POINT *ptOrigin)
{
	const Tab &tab = m_pexpp->GetTabContainer()->GetSelectedTab();
	ViewMode viewMode = tab.GetShellBrowser()->GetViewMode();

	HMENU viewsMenu = m_pexpp->BuildViewsMenu();

	int itemToCheck = GetViewModeMenuId(viewMode);
	CheckMenuRadioItem(viewsMenu, IDM_VIEW_THUMBNAILS, IDM_VIEW_EXTRALARGEICONS,
		itemToCheck, MF_BYCOMMAND);

	TrackPopupMenu(viewsMenu, TPM_LEFTALIGN, ptOrigin->x, ptOrigin->y,
		0, m_hwnd, nullptr);
}

// For some of the buttons on the toolbar, their state depends on an item from
// the application configuration, rather than the properties of the current tab
// or file selection.
void MainToolbar::UpdateConfigDependentButtonStates()
{
	SendMessage(m_hwnd, TB_CHECKBUTTON, ToolbarButton::Folders, m_config->showFolders);
}

void MainToolbar::UpdateToolbarButtonStates()
{
	const Tab &tab = m_pexpp->GetTabContainer()->GetSelectedTab();

	SendMessage(m_hwnd, TB_ENABLEBUTTON, ToolbarButton::Back, tab.GetShellBrowser()->GetNavigationController()->CanGoBack());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, ToolbarButton::Forward, tab.GetShellBrowser()->GetNavigationController()->CanGoForward());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, ToolbarButton::Up, tab.GetShellBrowser()->GetNavigationController()->CanGoUp());

	BOOL bVirtualFolder = tab.GetShellBrowser()->InVirtualFolder();

	SendMessage(m_hwnd, TB_ENABLEBUTTON, ToolbarButton::CopyTo, m_pexpp->CanCopy() && GetFocus() != m_pexpp->GetTreeView());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, ToolbarButton::MoveTo, m_pexpp->CanCut() && GetFocus() != m_pexpp->GetTreeView());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, ToolbarButton::Copy, m_pexpp->CanCopy());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, ToolbarButton::Cut, m_pexpp->CanCut());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, ToolbarButton::Paste, m_pexpp->CanPaste());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, ToolbarButton::Properties, m_pexpp->CanShowFileProperties());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, ToolbarButton::Delete, m_pexpp->CanDelete());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, ToolbarButton::DeletePermanently, m_pexpp->CanDelete());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, ToolbarButton::SplitFile, tab.GetShellBrowser()->GetNumSelectedFiles() == 1);
	SendMessage(m_hwnd, TB_ENABLEBUTTON, ToolbarButton::MergeFiles, tab.GetShellBrowser()->GetNumSelectedFiles() > 1);
	SendMessage(m_hwnd, TB_ENABLEBUTTON, ToolbarButton::OpenCommandPrompt, !bVirtualFolder);
	SendMessage(m_hwnd, TB_ENABLEBUTTON, ToolbarButton::NewFolder, m_pexpp->CanCreate());
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

void MainToolbar::OnFocusChanged(WindowFocusSource windowFocusSource)
{
	UNREFERENCED_PARAMETER(windowFocusSource);

	UpdateToolbarButtonStates();
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
	std::vector<ToolbarButton> toolbarButtons;

	wil::com_ptr<IXMLDOMNamedNodeMap> am;
	pNode->get_attributes(&am);

	long lChildNodes;
	am->get_length(&lChildNodes);

	for (long j = 1; j < lChildNodes; j++)
	{
		wil::com_ptr<IXMLDOMNode> pChildNode;
		am->get_item(j, &pChildNode);

		wil::unique_bstr bstrValue;
		pChildNode->get_text(&bstrValue);

		auto itr = TOOLBAR_BUTTON_XML_NAME_MAPPINGS.right.find(bstrValue.get());

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