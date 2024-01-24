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
#include "ShellItemsMenu.h"
#include "TabContainer.h"
#include "../Helper/Controls.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/Helper.h"
#include "../Helper/ImageHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/XMLSettings.h"
#include <boost/bimap.hpp>

// Enable C4062: enumerator 'identifier' in switch of enum 'enumeration' is not handled
#pragma warning(default : 4062)

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

// clang-format off
const std::unordered_map<MainToolbarButton, Icon, ToolbarButtonHash> TOOLBAR_BUTTON_ICON_MAPPINGS = {
	{MainToolbarButton::Back, Icon::Back},
	{MainToolbarButton::Forward, Icon::Forward},
	{MainToolbarButton::Up, Icon::Up},
	{MainToolbarButton::Folders, Icon::FolderTree},
	{MainToolbarButton::CopyTo, Icon::CopyTo},
	{MainToolbarButton::MoveTo, Icon::MoveTo},
	{MainToolbarButton::NewFolder, Icon::NewFolder},
	{MainToolbarButton::Copy, Icon::Copy},
	{MainToolbarButton::Cut, Icon::Cut},
	{MainToolbarButton::Paste, Icon::Paste},
	{MainToolbarButton::Delete, Icon::Delete},
	{MainToolbarButton::Views, Icon::Views},
	{MainToolbarButton::Search, Icon::Search},
	{MainToolbarButton::Properties, Icon::Properties},
	{MainToolbarButton::Refresh, Icon::Refresh},
	{MainToolbarButton::AddBookmark, Icon::AddBookmark},
	{MainToolbarButton::NewTab, Icon::NewTab},
	{MainToolbarButton::OpenCommandPrompt, Icon::CommandLine},
	{MainToolbarButton::Bookmarks, Icon::Bookmarks},
	{MainToolbarButton::DeletePermanently, Icon::DeletePermanently},
	{MainToolbarButton::SplitFile, Icon::SplitFiles},
	{MainToolbarButton::MergeFiles, Icon::MergeFiles},
	{MainToolbarButton::CloseTab, Icon::CloseTab}
};
// clang-format on

#pragma warning(push)
#pragma warning(                                                                                   \
		disable : 4996 4834) // warning STL4010: Various members of std::allocator are
							 // deprecated in C++17,
							 // discarding return value of function with 'nodiscard' attribute

// Ideally, toolbar button IDs would be saved in the XML config file, rather
// than button strings, but that's not especially easy to change now.
// clang-format off
const boost::bimap<MainToolbarButton, std::wstring> TOOLBAR_BUTTON_XML_NAME_MAPPINGS = MakeBimap<MainToolbarButton, std::wstring>({
	{MainToolbarButton::Back, L"Back"},
	{MainToolbarButton::Forward, L"Forward"},
	{MainToolbarButton::Up, L"Up"},
	{MainToolbarButton::Folders, L"Folders"},
	{MainToolbarButton::CopyTo, L"Copy To"},
	{MainToolbarButton::MoveTo, L"Move To"},
	{MainToolbarButton::NewFolder, L"New Folder"},
	{MainToolbarButton::Copy, L"Copy"},
	{MainToolbarButton::Cut, L"Cut"},
	{MainToolbarButton::Paste, L"Paste"},
	{MainToolbarButton::Delete, L"Delete"},
	{MainToolbarButton::Views, L"Views"},
	{MainToolbarButton::Search, L"Search"},
	{MainToolbarButton::Properties, L"Properties"},
	{MainToolbarButton::Refresh, L"Refresh"},
	{MainToolbarButton::AddBookmark, L"Bookmark the current tab"},
	{MainToolbarButton::NewTab, L"Create a new tab"},
	{MainToolbarButton::OpenCommandPrompt, L"Open Command Prompt"},
	{MainToolbarButton::Bookmarks, L"Organize Bookmarks"},
	{MainToolbarButton::DeletePermanently, L"Delete Permanently"},
	{MainToolbarButton::SplitFile, L"Split File"},
	{MainToolbarButton::MergeFiles, L"Merge Files"},
	{MainToolbarButton::CloseTab, L"Close Tab"},

	{MainToolbarButton::Separator, L"Separator"}
});
// clang-format on

#pragma warning(pop)

MainToolbar *MainToolbar::Create(HWND parent, HINSTANCE resourceInstance,
	CoreInterface *coreInterface, Navigator *navigator, IconFetcher *iconFetcher,
	std::shared_ptr<Config> config)
{
	return new MainToolbar(parent, resourceInstance, coreInterface, navigator, iconFetcher, config);
}

MainToolbar::MainToolbar(HWND parent, HINSTANCE resourceInstance, CoreInterface *coreInterface,
	Navigator *navigator, IconFetcher *iconFetcher, std::shared_ptr<Config> config) :
	BaseWindow(CreateMainToolbar(parent)),
	m_persistentSettings(&MainToolbarPersistentSettings::GetInstance()),
	m_resourceInstance(resourceInstance),
	m_coreInterface(coreInterface),
	m_navigator(navigator),
	m_iconFetcher(iconFetcher),
	m_config(config),
	m_fontSetter(m_hwnd, config.get()),
	m_tooltipFontSetter(reinterpret_cast<HWND>(SendMessage(m_hwnd, TB_GETTOOLTIPS, 0, 0)),
		config.get())
{
	Initialize(parent);
}

HWND MainToolbar::CreateMainToolbar(HWND parent)
{
	return CreateToolbar(parent,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS | TBSTYLE_LIST
			| TBSTYLE_TRANSPARENT | TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NORESIZE | CCS_ADJUSTABLE,
		TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_DOUBLEBUFFER
			| TBSTYLE_EX_HIDECLIPPEDBUTTONS);
}

void MainToolbar::Initialize(HWND parent)
{
	// Ideally, this constraint would be checked at compile-time, but the size
	// of TOOLBAR_BUTTON_ICON_MAPPINGS isn't known at compile-time. Note that
	// the MainToolbarButton enum contains one additional item - for the separator.
	assert(TOOLBAR_BUTTON_ICON_MAPPINGS.size() == (MainToolbarButton::_size() - 1));

	SendMessage(m_hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

	SHGetImageList(SHIL_SYSSMALL, IID_PPV_ARGS(&m_systemImageList));

	int defaultFolderIconIndex;
	FAIL_FAST_IF_FAILED(GetDefaultFolderIconIndex(defaultFolderIconIndex));
	m_defaultFolderIconBitmap =
		ImageHelper::ImageListIconToBitmap(m_systemImageList.get(), defaultFolderIconIndex);

	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(m_hwnd);

	int dpiScaledSizeSmall = MulDiv(TOOLBAR_IMAGE_SIZE_SMALL, dpi, USER_DEFAULT_SCREEN_DPI);
	int dpiScaledSizeLarge = MulDiv(TOOLBAR_IMAGE_SIZE_LARGE, dpi, USER_DEFAULT_SCREEN_DPI);

	m_imageListSmall.reset(ImageList_Create(dpiScaledSizeSmall, dpiScaledSizeSmall,
		ILC_COLOR32 | ILC_MASK, 0, static_cast<int>(MainToolbarButton::_size() - 1)));
	m_imageListLarge.reset(ImageList_Create(dpiScaledSizeLarge, dpiScaledSizeLarge,
		ILC_COLOR32 | ILC_MASK, 0, static_cast<int>(MainToolbarButton::_size() - 1)));

	m_toolbarImageMapSmall = SetUpToolbarImageList(m_imageListSmall.get(),
		m_coreInterface->GetIconResourceLoader(), TOOLBAR_IMAGE_SIZE_SMALL, dpi);
	m_toolbarImageMapLarge = SetUpToolbarImageList(m_imageListLarge.get(),
		m_coreInterface->GetIconResourceLoader(), TOOLBAR_IMAGE_SIZE_LARGE, dpi);

	SetTooolbarImageList();
	AddStringsToToolbar();
	AddButtonsToToolbar(m_persistentSettings->m_toolbarButtons);
	UpdateConfigDependentButtonStates();

	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(m_hwnd,
		std::bind_front(&MainToolbar::WndProc, this)));
	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(parent,
		std::bind_front(&MainToolbar::ParentWndProc, this)));

	m_coreInterface->AddTabsInitializedObserver(
		[this]
		{
			m_connections.push_back(
				m_coreInterface->GetTabContainer()->tabSelectedSignal.AddObserver(
					std::bind_front(&MainToolbar::OnTabSelected, this)));
			m_connections.push_back(
				m_coreInterface->GetTabContainer()->tabNavigationCommittedSignal.AddObserver(
					std::bind_front(&MainToolbar::OnNavigationCommitted, this)));
		});

	m_connections.push_back(m_coreInterface->AddFocusChangeObserver(
		std::bind_front(&MainToolbar::OnFocusChanged, this)));
	m_connections.push_back(m_config->useLargeToolbarIcons.addObserver(
		std::bind_front(&MainToolbar::OnUseLargeToolbarIconsUpdated, this)));
	m_connections.push_back(m_config->showFolders.addObserver(
		std::bind_front(&MainToolbar::OnShowFoldersUpdated, this)));

	AddClipboardFormatListener(m_hwnd);

	m_fontSetter.fontUpdatedSignal.AddObserver(
		std::bind_front(&MainToolbar::OnFontOrDpiUpdated, this));
}

MainToolbar::~MainToolbar()
{
	RemoveClipboardFormatListener(m_hwnd);
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
		wil::unique_hbitmap bitmap =
			iconResourceLoader->LoadBitmapFromPNGForDpi(mapping.second, iconSize, iconSize, dpi);

		int imagePosition = ImageList_Add(imageList, bitmap.get(), nullptr);

		if (imagePosition == -1)
		{
			continue;
		}

		imageListMappings.insert({ mapping.first, imagePosition });
	}

	return imageListMappings;
}

LRESULT MainToolbar::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		HANDLE_MSG(hwnd, WM_MBUTTONDOWN, OnMButtonDown);
		HANDLE_MSG(hwnd, WM_MBUTTONUP, OnMButtonUp);

	case WM_CLIPBOARDUPDATE:
		OnClipboardUpdate();
		return 0;

	case WM_DPICHANGED_AFTERPARENT:
		OnFontOrDpiUpdated();
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT MainToolbar::ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

void MainToolbar::AddButtonsToToolbar(const std::vector<MainToolbarButton> &buttons)
{
	for (auto button : buttons)
	{
		AddButtonToToolbar(button);
	}
}

void MainToolbar::AddButtonToToolbar(MainToolbarButton button)
{
	TBBUTTON tbButton = GetToolbarButtonDetails(button);
	SendMessage(m_hwnd, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&tbButton));
}

TBBUTTON MainToolbar::GetToolbarButtonDetails(MainToolbarButton button) const
{
	TBBUTTON tbButton;

	ZeroMemory(&tbButton, sizeof(tbButton));

	if (button == +MainToolbarButton::Separator)
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
		BYTE standardStyle = BTNS_AUTOSIZE;

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
	for (auto button : MainToolbarButton::_values())
	{
		if (button == +MainToolbarButton::Separator)
		{
			continue;
		}

		AddStringToToolbar(button);
	}
}

void MainToolbar::AddStringToToolbar(MainToolbarButton button)
{
	TCHAR szText[64];

	/* The string must be double NULL-terminated. */
	GetToolbarButtonText(button, szText, SIZEOF_ARRAY(szText));
	szText[lstrlen(szText) + 1] = '\0';

	int index =
		static_cast<int>(SendMessage(m_hwnd, TB_ADDSTRING, NULL, reinterpret_cast<LPARAM>(szText)));

	m_toolbarStringMap.insert(std::make_pair(button, index));
}

void MainToolbar::GetToolbarButtonText(MainToolbarButton button, TCHAR *szText, int bufSize) const
{
	int res = LoadString(m_resourceInstance, LookupToolbarButtonTextID(button), szText, bufSize);
	assert(res != 0);

	/* It doesn't really make sense to return this. If the string isn't in the
	string table, there's a bug somewhere in the program. */
	UNUSED(res);
}

BYTE MainToolbar::LookupToolbarButtonExtraStyles(MainToolbarButton button) const
{
	switch (button)
	{
	case MainToolbarButton::Back:
		return BTNS_DROPDOWN;

	case MainToolbarButton::Forward:
		return BTNS_DROPDOWN;

	case MainToolbarButton::Up:
		return BTNS_DROPDOWN;

	case MainToolbarButton::Folders:
		return BTNS_SHOWTEXT | BTNS_CHECK;

	case MainToolbarButton::Views:
		return BTNS_DROPDOWN;

	default:
		return 0;
	}
}

int MainToolbar::LookupToolbarButtonTextID(MainToolbarButton button) const
{
	switch (button)
	{
	case MainToolbarButton::Separator:
		return IDS_SEPARATOR;

	case MainToolbarButton::Back:
		return IDS_TOOLBAR_BACK;

	case MainToolbarButton::Forward:
		return IDS_TOOLBAR_FORWARD;

	case MainToolbarButton::Up:
		return IDS_TOOLBAR_UP;

	case MainToolbarButton::Folders:
		return IDS_TOOLBAR_FOLDERS;

	case MainToolbarButton::CopyTo:
		return IDS_TOOLBAR_COPYTO;

	case MainToolbarButton::MoveTo:
		return IDS_TOOLBAR_MOVETO;

	case MainToolbarButton::NewFolder:
		return IDS_TOOLBAR_NEWFOLDER;

	case MainToolbarButton::Copy:
		return IDS_TOOLBAR_COPY;

	case MainToolbarButton::Cut:
		return IDS_TOOLBAR_CUT;

	case MainToolbarButton::Paste:
		return IDS_TOOLBAR_PASTE;

	case MainToolbarButton::Delete:
		return IDS_TOOLBAR_DELETE;

	case MainToolbarButton::DeletePermanently:
		return IDS_TOOLBAR_DELETEPERMANENTLY;

	case MainToolbarButton::Views:
		return IDS_TOOLBAR_VIEWS;

	case MainToolbarButton::Search:
		return IDS_TOOLBAR_SEARCH;

	case MainToolbarButton::Properties:
		return IDS_TOOLBAR_PROPERTIES;

	case MainToolbarButton::Refresh:
		return IDS_TOOLBAR_REFRESH;

	case MainToolbarButton::AddBookmark:
		return IDS_TOOLBAR_ADDBOOKMARK;

	case MainToolbarButton::Bookmarks:
		return IDS_TOOLBAR_MANAGEBOOKMARKS;

	case MainToolbarButton::NewTab:
		return IDS_TOOLBAR_NEWTAB;

	case MainToolbarButton::OpenCommandPrompt:
		return IDS_TOOLBAR_OPENCOMMANDPROMPT;

	case MainToolbarButton::SplitFile:
		return IDS_TOOLBAR_SPLIT_FILE;

	case MainToolbarButton::MergeFiles:
		return IDS_TOOLBAR_MERGE_FILES;

	case MainToolbarButton::CloseTab:
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

	sizeUpdatedSignal.m_signal();
}

void MainToolbar::UpdateToolbarButtonImageIndexes()
{
	int numButtons = static_cast<int>(SendMessage(m_hwnd, TB_BUTTONCOUNT, 0, 0));

	for (int i = 0; i < numButtons; i++)
	{
		TBBUTTON tbButton;
		BOOL res = static_cast<BOOL>(
			SendMessage(m_hwnd, TB_GETBUTTON, i, reinterpret_cast<LPARAM>(&tbButton)));

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

void MainToolbar::OnShowFoldersUpdated(bool showFolders)
{
	SendMessage(m_hwnd, TB_CHECKBUTTON, MainToolbarButton::Folders, showFolders);
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

	if ((pnmtb->iItem >= 0)
		&& (static_cast<std::size_t>(pnmtb->iItem) < (MainToolbarButton::_size() - 1)))
	{
		// Note that the separator (which is the first item in the enumeration)
		// is skipped.
		assert(MainToolbarButton::_values()[0] == +MainToolbarButton::Separator);
		MainToolbarButton button = MainToolbarButton::_values()[pnmtb->iItem + 1];
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

	m_persistentSettings->m_toolbarButtons = { DEFAULT_TOOLBAR_BUTTONS,
		std::end(DEFAULT_TOOLBAR_BUTTONS) };

	AddButtonsToToolbar(m_persistentSettings->m_toolbarButtons);
	UpdateConfigDependentButtonStates();
	UpdateToolbarButtonStates();
}

void MainToolbar::OnTBChange()
{
	std::vector<MainToolbarButton> toolbarButtons;
	int numButtons = static_cast<int>(SendMessage(m_hwnd, TB_BUTTONCOUNT, 0, 0));

	for (int i = 0; i < numButtons; i++)
	{
		TBBUTTON tbButton;
		BOOL res = static_cast<BOOL>(
			SendMessage(m_hwnd, TB_GETBUTTON, i, reinterpret_cast<LPARAM>(&tbButton)));

		if (!res)
		{
			continue;
		}

		int id;

		if (tbButton.idCommand == 0)
		{
			id = MainToolbarButton::Separator;
		}
		else
		{
			id = tbButton.idCommand;
		}

		toolbarButtons.push_back(MainToolbarButton::_from_integral(id));
	}

	m_persistentSettings->m_toolbarButtons = toolbarButtons;

	UpdateConfigDependentButtonStates();
}

void MainToolbar::OnTBGetInfoTip(LPARAM lParam)
{
	auto *ptbgit = reinterpret_cast<NMTBGETINFOTIP *>(lParam);

	StringCchCopy(ptbgit->pszText, ptbgit->cchTextMax, EMPTY_STRING);

	const Tab &tab = m_coreInterface->GetTabContainer()->GetSelectedTab();

	if (ptbgit->iItem == MainToolbarButton::Back)
	{
		auto entry = tab.GetShellBrowser()->GetNavigationController()->GetEntry(-1);

		if (entry)
		{
			TCHAR szInfoTip[1024];
			TCHAR szTemp[64];
			LoadString(m_resourceInstance, IDS_MAIN_TOOLBAR_BACK, szTemp, SIZEOF_ARRAY(szTemp));
			StringCchPrintf(szInfoTip, SIZEOF_ARRAY(szInfoTip), szTemp,
				entry->GetDisplayName().c_str());

			StringCchCopy(ptbgit->pszText, ptbgit->cchTextMax, szInfoTip);
		}
	}
	else if (ptbgit->iItem == MainToolbarButton::Forward)
	{
		auto entry = tab.GetShellBrowser()->GetNavigationController()->GetEntry(1);

		if (entry)
		{
			TCHAR szInfoTip[1024];
			TCHAR szTemp[64];
			LoadString(m_resourceInstance, IDS_MAIN_TOOLBAR_FORWARD, szTemp, SIZEOF_ARRAY(szTemp));
			StringCchPrintf(szInfoTip, SIZEOF_ARRAY(szInfoTip), szTemp,
				entry->GetDisplayName().c_str());

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

	if (nmtb->iItem == MainToolbarButton::Back)
	{
		ShowHistoryMenu(HistoryType::Back, ptOrigin);

		return TBDDRET_DEFAULT;
	}
	else if (nmtb->iItem == MainToolbarButton::Forward)
	{
		RECT backButtonRect;
		SendMessage(m_hwnd, TB_GETRECT, MainToolbarButton::Back,
			reinterpret_cast<LPARAM>(&backButtonRect));

		ptOrigin.x += backButtonRect.right;

		ShowHistoryMenu(HistoryType::Forward, ptOrigin);

		return TBDDRET_DEFAULT;
	}
	else if (nmtb->iItem == MainToolbarButton::Up)
	{
		ShowUpNavigationDropdown();
		return TBDDRET_DEFAULT;
	}
	else if (nmtb->iItem == MainToolbarButton::Views)
	{
		ShowToolbarViewsDropdown();

		return TBDDRET_DEFAULT;
	}

	return TBDDRET_NODEFAULT;
}

void MainToolbar::ShowHistoryMenu(HistoryType historyType, const POINT &pt)
{
	std::vector<HistoryEntry *> history;

	const Tab &tab = m_coreInterface->GetTabContainer()->GetSelectedTab();

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
			wil::unique_hbitmap iconBitmap =
				ImageHelper::ImageListIconToBitmap(m_systemImageList.get(), *iconIndex);

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

	int cmd = TrackPopupMenu(menu.get(), TPM_LEFTALIGN | TPM_VERTICAL | TPM_RETURNCMD, pt.x, pt.y,
		0, m_hwnd, nullptr);

	if (cmd == 0)
	{
		return;
	}

	if (historyType == HistoryType::Back)
	{
		cmd = -cmd;
	}

	Tab &selectedTab = m_coreInterface->GetTabContainer()->GetSelectedTab();
	selectedTab.GetShellBrowser()->GetNavigationController()->GoToOffset(cmd);
}

void MainToolbar::ShowUpNavigationDropdown()
{
	const Tab &tab = m_coreInterface->GetTabContainer()->GetSelectedTab();
	auto pidl = tab.GetShellBrowser()->GetDirectoryIdl();

	auto parentPidls = GetParentPidlCollection(pidl.get());

	if (parentPidls.empty())
	{
		// This function should never be called when displaying the root folder.
		assert(false);
		return;
	}

	// Items in the menu will be displayed in the same order they appear in the vector.
	// GetParentPidlCollection() will return a set of items that starts from the parent and proceeds
	// to the root. In the menu, the root needs to be shown first.
	std::reverse(parentPidls.begin(), parentPidls.end());

	RECT rcButton;
	[[maybe_unused]] auto res =
		SendMessage(m_hwnd, TB_GETRECT, MainToolbarButton::Up, reinterpret_cast<LPARAM>(&rcButton));
	assert(res);

	POINT pt = { rcButton.left, rcButton.bottom };
	res = ClientToScreen(m_hwnd, &pt);
	assert(res);

	ShellItemsMenu menu(parentPidls, m_navigator, m_iconFetcher);
	menu.Show(m_hwnd, pt);
}

void MainToolbar::ShowToolbarViewsDropdown()
{
	POINT ptOrigin;
	RECT rcButton;

	SendMessage(m_hwnd, TB_GETRECT, (WPARAM) MainToolbarButton::Views, (LPARAM) &rcButton);

	ptOrigin.x = rcButton.left;
	ptOrigin.y = rcButton.bottom;

	ClientToScreen(m_hwnd, &ptOrigin);

	CreateViewsMenu(&ptOrigin);
}

void MainToolbar::CreateViewsMenu(POINT *ptOrigin)
{
	auto viewsMenu = m_coreInterface->BuildViewsMenu();
	TrackPopupMenu(viewsMenu.get(), TPM_LEFTALIGN, ptOrigin->x, ptOrigin->y, 0, m_hwnd, nullptr);
}

// For some of the buttons on the toolbar, their state depends on an item from
// the application configuration, rather than the properties of the current tab
// or file selection.
void MainToolbar::UpdateConfigDependentButtonStates()
{
	SendMessage(m_hwnd, TB_CHECKBUTTON, MainToolbarButton::Folders, m_config->showFolders.get());
}

void MainToolbar::UpdateToolbarButtonStates()
{
	const Tab &tab = m_coreInterface->GetTabContainer()->GetSelectedTab();

	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::Back,
		tab.GetShellBrowser()->GetNavigationController()->CanGoBack());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::Forward,
		tab.GetShellBrowser()->GetNavigationController()->CanGoForward());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::Up,
		tab.GetShellBrowser()->GetNavigationController()->CanGoUp());

	bool virtualFolder = tab.GetShellBrowser()->InVirtualFolder();

	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::CopyTo,
		m_coreInterface->CanCopy() && GetFocus() != m_coreInterface->GetTreeView());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::MoveTo,
		m_coreInterface->CanCut() && GetFocus() != m_coreInterface->GetTreeView());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::Copy, m_coreInterface->CanCopy());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::Cut, m_coreInterface->CanCut());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::Paste, m_coreInterface->CanPaste());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::Properties,
		m_coreInterface->CanShowFileProperties());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::Delete, m_coreInterface->CanDelete());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::DeletePermanently,
		m_coreInterface->CanDelete());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::SplitFile,
		tab.GetShellBrowser()->GetNumSelectedFiles() == 1);
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::MergeFiles,
		tab.GetShellBrowser()->GetNumSelectedFiles() > 1);
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::OpenCommandPrompt, !virtualFolder);
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::NewFolder,
		m_coreInterface->CanCreate());
}

void MainToolbar::OnClipboardUpdate()
{
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::Paste, m_coreInterface->CanPaste());
}

void MainToolbar::OnMButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, UINT keysDown)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(doubleClick);
	UNREFERENCED_PARAMETER(keysDown);

	POINT pt = { x, y };
	int index = static_cast<int>(SendMessage(m_hwnd, TB_HITTEST, 0, reinterpret_cast<LPARAM>(&pt)));

	if (index >= 0)
	{
		m_middleButtonItem = index;
	}
	else
	{
		m_middleButtonItem.reset();
	}
}

void MainToolbar::OnMButtonUp(HWND hwnd, int x, int y, UINT keysDown)
{
	UNREFERENCED_PARAMETER(hwnd);

	POINT pt = { x, y };
	int index = static_cast<int>(SendMessage(m_hwnd, TB_HITTEST, 0, reinterpret_cast<LPARAM>(&pt)));

	if (index < 0 || !m_middleButtonItem || index != *m_middleButtonItem)
	{
		return;
	}

	TBBUTTON tbButton;
	BOOL res = static_cast<BOOL>(
		SendMessage(m_hwnd, TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&tbButton)));

	if (!res)
	{
		return;
	}

	if (tbButton.idCommand == 0)
	{
		// Separator.
		return;
	}

	const Tab &tab = m_coreInterface->GetTabContainer()->GetSelectedTab();
	unique_pidl_absolute pidl;

	if (tbButton.idCommand == MainToolbarButton::Back
		|| tbButton.idCommand == MainToolbarButton::Forward)
	{
		HistoryEntry *entry = nullptr;

		if (tbButton.idCommand == MainToolbarButton::Back)
		{
			entry = tab.GetShellBrowser()->GetNavigationController()->GetEntry(-1);
		}
		else
		{
			entry = tab.GetShellBrowser()->GetNavigationController()->GetEntry(1);
		}

		if (!entry)
		{
			return;
		}

		pidl = entry->GetPidl();
	}
	else if (tbButton.idCommand == MainToolbarButton::Up)
	{
		auto *currentEntry = tab.GetShellBrowser()->GetNavigationController()->GetCurrentEntry();

		unique_pidl_absolute pidlParent;
		HRESULT hr =
			GetVirtualParentPath(currentEntry->GetPidl().get(), wil::out_param(pidlParent));

		if (FAILED(hr))
		{
			return;
		}

		pidl = std::move(pidlParent);
	}

	if (!pidl)
	{
		return;
	}

	bool switchToNewTab = m_config->openTabsInForeground;

	if (WI_IsFlagSet(keysDown, MK_SHIFT))
	{
		switchToNewTab = !switchToNewTab;
	}

	auto navigateParams = NavigateParams::Normal(pidl.get());
	m_coreInterface->GetTabContainer()->CreateNewTab(navigateParams,
		TabSettings(_selected = switchToNewTab));
}

void MainToolbar::OnTabSelected(const Tab &tab)
{
	UNREFERENCED_PARAMETER(tab);

	UpdateToolbarButtonStates();
}

void MainToolbar::OnNavigationCommitted(const Tab &tab, const NavigateParams &navigateParams)
{
	UNREFERENCED_PARAMETER(navigateParams);

	if (m_coreInterface->GetTabContainer()->IsTabSelected(tab))
	{
		UpdateToolbarButtonStates();
	}
}

void MainToolbar::OnFocusChanged()
{
	UpdateToolbarButtonStates();
}

MainToolbarPersistentSettings::MainToolbarPersistentSettings() :
	m_toolbarButtons(DEFAULT_TOOLBAR_BUTTONS, std::end(DEFAULT_TOOLBAR_BUTTONS))
{
	assert(TOOLBAR_BUTTON_XML_NAME_MAPPINGS.size() == MainToolbarButton::_size());
}

MainToolbarPersistentSettings &MainToolbarPersistentSettings::GetInstance()
{
	static MainToolbarPersistentSettings persistentSettings;
	return persistentSettings;
}

void MainToolbarPersistentSettings::LoadXMLSettings(IXMLDOMNode *pNode)
{
	std::vector<MainToolbarButton> toolbarButtons;

	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> am;
	pNode->get_attributes(&am);

	long lChildNodes;
	am->get_length(&lChildNodes);

	for (long j = 1; j < lChildNodes; j++)
	{
		wil::com_ptr_nothrow<IXMLDOMNode> pChildNode;
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
		StringCchPrintf(szButtonAttributeName, SIZEOF_ARRAY(szButtonAttributeName), _T("Button%d"),
			index);

		std::wstring buttonName = TOOLBAR_BUTTON_XML_NAME_MAPPINGS.left.at(button);

		NXMLSettings::AddAttributeToNode(pXMLDom, pe, szButtonAttributeName, buttonName.c_str());

		index++;
	}
}

void MainToolbar::OnFontOrDpiUpdated()
{
	RefreshToolbarAfterFontOrDpiChange(m_hwnd);

	sizeUpdatedSignal.m_signal();
}
