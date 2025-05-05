// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainToolbar.h"
#include "App.h"
#include "BrowserCommandController.h"
#include "Config.h"
#include "DefaultToolbarButtons.h"
#include "Icon.h"
#include "MainResource.h"
#include "NavigationHelper.h"
#include "PopupMenuView.h"
#include "ResourceLoader.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "TabContainerImpl.h"
#include "TabParentItemsMenu.h"
#include "../Helper/Controls.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/Helper.h"
#include "../Helper/ImageHelper.h"
#include <fmt/format.h>
#include <fmt/xchar.h>

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

MainToolbar *MainToolbar::Create(HWND parent, App *app, BrowserWindow *browser,
	CoreInterface *coreInterface, const ResourceLoader *resourceLoader,
	ShellIconLoader *shellIconLoader,
	const std::optional<MainToolbarStorage::MainToolbarButtons> &initialButtons)
{
	return new MainToolbar(parent, app, browser, coreInterface, resourceLoader, shellIconLoader,
		initialButtons);
}

MainToolbar::MainToolbar(HWND parent, App *app, BrowserWindow *browser,
	CoreInterface *coreInterface, const ResourceLoader *resourceLoader,
	ShellIconLoader *shellIconLoader,
	const std::optional<MainToolbarStorage::MainToolbarButtons> &initialButtons) :
	BaseWindow(CreateMainToolbar(parent)),
	m_app(app),
	m_browser(browser),
	m_coreInterface(coreInterface),
	m_resourceLoader(resourceLoader),
	m_shellIconLoader(shellIconLoader),
	m_fontSetter(m_hwnd, m_app->GetConfig()),
	m_tooltipFontSetter(reinterpret_cast<HWND>(SendMessage(m_hwnd, TB_GETTOOLTIPS, 0, 0)),
		m_app->GetConfig())
{
	Initialize(parent, initialButtons);
}

HWND MainToolbar::CreateMainToolbar(HWND parent)
{
	return CreateToolbar(parent,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS | TBSTYLE_LIST
			| TBSTYLE_TRANSPARENT | TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NORESIZE | CCS_ADJUSTABLE,
		TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_DOUBLEBUFFER
			| TBSTYLE_EX_HIDECLIPPEDBUTTONS);
}

void MainToolbar::Initialize(HWND parent,
	const std::optional<MainToolbarStorage::MainToolbarButtons> &initialButtons)
{
	// Ideally, this constraint would be checked at compile-time, but the size
	// of TOOLBAR_BUTTON_ICON_MAPPINGS isn't known at compile-time. Note that
	// the MainToolbarButton enum contains one additional item - for the separator.
	assert(TOOLBAR_BUTTON_ICON_MAPPINGS.size() == (MainToolbarButton::_size() - 1));

	SendMessage(m_hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(m_hwnd);

	int dpiScaledSizeSmall = MulDiv(TOOLBAR_IMAGE_SIZE_SMALL, dpi, USER_DEFAULT_SCREEN_DPI);
	int dpiScaledSizeLarge = MulDiv(TOOLBAR_IMAGE_SIZE_LARGE, dpi, USER_DEFAULT_SCREEN_DPI);

	m_imageListSmall.reset(ImageList_Create(dpiScaledSizeSmall, dpiScaledSizeSmall,
		ILC_COLOR32 | ILC_MASK, 0, static_cast<int>(MainToolbarButton::_size() - 1)));
	m_imageListLarge.reset(ImageList_Create(dpiScaledSizeLarge, dpiScaledSizeLarge,
		ILC_COLOR32 | ILC_MASK, 0, static_cast<int>(MainToolbarButton::_size() - 1)));

	m_toolbarImageMapSmall =
		SetUpToolbarImageList(m_imageListSmall.get(), TOOLBAR_IMAGE_SIZE_SMALL, dpi);
	m_toolbarImageMapLarge =
		SetUpToolbarImageList(m_imageListLarge.get(), TOOLBAR_IMAGE_SIZE_LARGE, dpi);

	SetTooolbarImageList();
	AddButtonsToToolbar(initialButtons ? initialButtons->GetButtons() : GetDefaultButtons());
	UpdateConfigDependentButtonStates();

	m_windowSubclasses.push_back(
		std::make_unique<WindowSubclass>(m_hwnd, std::bind_front(&MainToolbar::WndProc, this)));
	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(parent,
		std::bind_front(&MainToolbar::ParentWndProc, this)));

	m_connections.push_back(m_browser->AddLifecycleStateChangedObserver(
		std::bind_front(&MainToolbar::OnBrowserLifecycleStateChanged, this)));

	m_connections.push_back(m_app->GetTabEvents()->AddSelectedObserver(
		std::bind_front(&MainToolbar::OnTabSelected, this), TabEventScope::ForBrowser(*m_browser)));

	m_connections.push_back(m_app->GetNavigationEvents()->AddCommittedObserver(
		std::bind_front(&MainToolbar::OnNavigationCommitted, this),
		NavigationEventScope::ForActiveShellBrowser(*m_browser)));

	m_connections.push_back(m_browser->GetCommandTargetManager()->targetChangedSignal.AddObserver(
		std::bind_front(&MainToolbar::OnBrowserCommandTargetChanged, this)));
	m_connections.push_back(m_app->GetConfig()->useLargeToolbarIcons.addObserver(
		std::bind_front(&MainToolbar::OnUseLargeToolbarIconsUpdated, this)));
	m_connections.push_back(m_app->GetConfig()->showFolders.addObserver(
		std::bind_front(&MainToolbar::OnShowFoldersUpdated, this)));

	m_connections.push_back(m_app->GetClipboardWatcher()->updateSignal.AddObserver(
		std::bind_front(&MainToolbar::OnClipboardUpdate, this)));

	m_fontSetter.fontUpdatedSignal.AddObserver(
		std::bind_front(&MainToolbar::OnFontOrDpiUpdated, this));
}

void MainToolbar::SetTooolbarImageList()
{
	HIMAGELIST himl;

	if (m_app->GetConfig()->useLargeToolbarIcons.get())
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

std::unordered_map<int, int> MainToolbar::SetUpToolbarImageList(HIMAGELIST imageList, int iconSize,
	UINT dpi)
{
	std::unordered_map<int, int> imageListMappings;

	for (const auto &mapping : TOOLBAR_BUTTON_ICON_MAPPINGS)
	{
		wil::unique_hbitmap bitmap =
			m_resourceLoader->LoadBitmapFromPNGForDpi(mapping.second, iconSize, iconSize, dpi);

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

std::vector<MainToolbarButton> MainToolbar::GetDefaultButtons() const
{
	return { std::begin(DEFAULT_TOOLBAR_BUTTONS), std::end(DEFAULT_TOOLBAR_BUTTONS) };
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
	if (button != +MainToolbarButton::Separator
		&& SendMessage(m_hwnd, TB_COMMANDTOINDEX, button, 0) != -1)
	{
		DCHECK(false) << "Toolbar button already exists";
		return;
	}

	TBBUTTON tbButton = GetToolbarButtonDetails(button);
	auto buttonText = GetToolbarButtonText(button);
	tbButton.iString = reinterpret_cast<INT_PTR>(buttonText.c_str());
	SendMessage(m_hwnd, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&tbButton));
}

// Returns the button details, excluding the button text, since the TBBUTTON struct only includes
// space for a pointer to the text, which would create potential lifetime issues.
TBBUTTON MainToolbar::GetToolbarButtonDetails(MainToolbarButton button) const
{
	TBBUTTON tbButton = {};

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

		int imagePosition;

		if (m_app->GetConfig()->useLargeToolbarIcons.get())
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
		tbButton.iString = 0;
	}

	return tbButton;
}

std::wstring MainToolbar::GetToolbarButtonText(MainToolbarButton button) const
{
	return m_resourceLoader->LoadString(LookupToolbarButtonTextID(button));
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

void MainToolbar::OnBrowserLifecycleStateChanged(BrowserWindow::LifecycleState updatedState)
{
	if (updatedState == BrowserWindow::LifecycleState::Main)
	{
		UpdateToolbarButtonStates();
	}
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

		if (m_app->GetConfig()->useLargeToolbarIcons.get())
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

BOOL MainToolbar::OnTBGetButtonInfo(LPARAM lParam)
{
	auto *pnmtb = reinterpret_cast<NMTOOLBAR *>(lParam);

	if ((pnmtb->iItem >= 0)
		&& (static_cast<std::size_t>(pnmtb->iItem) < (MainToolbarButton::_size() - 1)))
	{
		// Note that the separator (which is the first item in the enumeration) is skipped.
		assert(MainToolbarButton::_values()[0] == +MainToolbarButton::Separator);
		MainToolbarButton button = MainToolbarButton::_values()[pnmtb->iItem + 1];
		pnmtb->tbButton = GetToolbarButtonDetails(button);

		auto buttonText = GetToolbarButtonText(button);
		StringCchCopy(pnmtb->pszText, pnmtb->cchText, buttonText.c_str());

		pnmtb->tbButton.iString = reinterpret_cast<INT_PTR>(pnmtb->pszText);

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

	AddButtonsToToolbar(GetDefaultButtons());
	UpdateConfigDependentButtonStates();
	UpdateToolbarButtonStates();
}

void MainToolbar::OnTBChange()
{
	UpdateConfigDependentButtonStates();
}

void MainToolbar::OnTBGetInfoTip(LPARAM lParam)
{
	auto *ptbgit = reinterpret_cast<NMTBGETINFOTIP *>(lParam);

	StringCchCopy(ptbgit->pszText, ptbgit->cchTextMax, L"");

	const auto *shellBrowser = m_browser->GetActiveShellBrowser();

	if (ptbgit->iItem == MainToolbarButton::Back)
	{
		auto entry = shellBrowser->GetNavigationController()->GetEntry(-1);

		if (entry)
		{
			std::wstring infoTipTemplate = m_resourceLoader->LoadString(IDS_MAIN_TOOLBAR_BACK);
			std::wstring infoTip = fmt::format(fmt::runtime(infoTipTemplate),
				fmt::arg(L"folder_name",
					GetDisplayNameWithFallback(entry->GetPidl().Raw(), SHGDN_INFOLDER)));
			StringCchCopy(ptbgit->pszText, ptbgit->cchTextMax, infoTip.c_str());
		}
	}
	else if (ptbgit->iItem == MainToolbarButton::Forward)
	{
		auto entry = shellBrowser->GetNavigationController()->GetEntry(1);

		if (entry)
		{
			std::wstring infoTipTemplate = m_resourceLoader->LoadString(IDS_MAIN_TOOLBAR_FORWARD);
			std::wstring infoTip = fmt::format(fmt::runtime(infoTipTemplate),
				fmt::arg(L"folder_name",
					GetDisplayNameWithFallback(entry->GetPidl().Raw(), SHGDN_INFOLDER)));
			StringCchCopy(ptbgit->pszText, ptbgit->cchTextMax, infoTip.c_str());
		}
	}
	else if (ptbgit->iItem == MainToolbarButton::Up)
	{
		auto customizedInfoTip = MaybeGetCustomizedUpInfoTip();

		if (customizedInfoTip)
		{
			StringCchCopy(ptbgit->pszText, ptbgit->cchTextMax, customizedInfoTip->c_str());
		}
	}
}

// If there's a parent folder to navigate up to, a customized infotip will be generated that
// contains the name of the folder.
std::optional<std::wstring> MainToolbar::MaybeGetCustomizedUpInfoTip()
{
	const auto *shellBrowser = m_browser->GetActiveShellBrowser();
	const auto *currentEntry = shellBrowser->GetNavigationController()->GetCurrentEntry();

	unique_pidl_absolute parentPidl;
	HRESULT hr = GetVirtualParentPath(currentEntry->GetPidl().Raw(), wil::out_param(parentPidl));

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	std::wstring parentName;
	hr = GetDisplayName(parentPidl.get(), SHGDN_NORMAL, parentName);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	std::wstring infoTipTemplate = m_resourceLoader->LoadString(IDS_MAIN_TOOLBAR_UP_TO_FOLDER);
	std::wstring infoTip =
		fmt::format(fmt::runtime(infoTipTemplate), fmt::arg(L"folder_name", parentName));

	return infoTip;
}

LRESULT MainToolbar::OnTbnDropDown(const NMTOOLBAR *nmtb)
{
	if (nmtb->iItem == MainToolbarButton::Back)
	{
		ShowHistoryMenu(TabHistoryMenu::MenuType::Back);
		return TBDDRET_DEFAULT;
	}
	else if (nmtb->iItem == MainToolbarButton::Forward)
	{
		ShowHistoryMenu(TabHistoryMenu::MenuType::Forward);
		return TBDDRET_DEFAULT;
	}
	else if (nmtb->iItem == MainToolbarButton::Up)
	{
		ShowUpNavigationMenu();
		return TBDDRET_DEFAULT;
	}
	else if (nmtb->iItem == MainToolbarButton::Views)
	{
		ShowToolbarViewsMenu();
		return TBDDRET_DEFAULT;
	}

	return TBDDRET_NODEFAULT;
}

void MainToolbar::ShowHistoryMenu(TabHistoryMenu::MenuType historyType)
{
	const auto *shellBrowser = m_browser->GetActiveShellBrowser();
	const auto *navigationController = shellBrowser->GetNavigationController();

	if ((historyType == TabHistoryMenu::MenuType::Back && !navigationController->CanGoBack())
		|| (historyType == TabHistoryMenu::MenuType::Forward
			&& !navigationController->CanGoForward()))
	{
		return;
	}

	MainToolbarButton button;

	if (historyType == TabHistoryMenu::MenuType::Back)
	{
		button = MainToolbarButton::Back;
	}
	else
	{
		button = MainToolbarButton::Forward;
	}

	PopupMenuView popupMenu;
	TabHistoryMenu menu(&popupMenu, m_app->GetAcceleratorManager(), m_browser, m_shellIconLoader,
		historyType);
	popupMenu.Show(m_hwnd, GetMenuPositionForButton(button));
}

void MainToolbar::ShowUpNavigationMenu()
{
	PopupMenuView popupMenu;
	TabParentItemsMenu menu(&popupMenu, m_app->GetAcceleratorManager(), m_browser,
		m_shellIconLoader);
	popupMenu.Show(m_hwnd, GetMenuPositionForButton(MainToolbarButton::Up));
}

void MainToolbar::ShowToolbarViewsMenu()
{
	auto viewsMenu = m_coreInterface->BuildViewsMenu();
	auto pt = GetMenuPositionForButton(MainToolbarButton::Views);
	TrackPopupMenu(viewsMenu.get(), TPM_LEFTALIGN, pt.x, pt.y, 0, m_hwnd, nullptr);
}

// Returns the position a menu should be anchored at for a particular toolbar button.
POINT MainToolbar::GetMenuPositionForButton(MainToolbarButton button)
{
	RECT rcButton;
	auto res = SendMessage(m_hwnd, TB_GETRECT, button, reinterpret_cast<LPARAM>(&rcButton));
	CHECK(res);

	POINT pt = { rcButton.left, rcButton.bottom };
	res = ClientToScreen(m_hwnd, &pt);
	CHECK(res);

	return pt;
}

// For some of the buttons on the toolbar, their state depends on an item from
// the application configuration, rather than the properties of the current tab
// or file selection.
void MainToolbar::UpdateConfigDependentButtonStates()
{
	SendMessage(m_hwnd, TB_CHECKBUTTON, MainToolbarButton::Folders,
		m_app->GetConfig()->showFolders.get());
}

void MainToolbar::UpdateToolbarButtonStates()
{
	if (m_browser->GetLifecycleState() != BrowserWindow::LifecycleState::Main)
	{
		return;
	}

	auto *browserController = m_browser->GetCommandController();
	const Tab &tab = m_coreInterface->GetTabContainerImpl()->GetSelectedTab();

	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::Back,
		browserController->IsCommandEnabled(IDM_GO_BACK));
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::Forward,
		browserController->IsCommandEnabled(IDM_GO_FORWARD));
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::Up,
		browserController->IsCommandEnabled(IDM_GO_UP));

	bool virtualFolder = tab.GetShellBrowserImpl()->InVirtualFolder();

	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::CopyTo,
		m_coreInterface->CanCopy() && GetFocus() != m_coreInterface->GetTreeView());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::MoveTo,
		m_coreInterface->CanCut() && GetFocus() != m_coreInterface->GetTreeView());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::Copy, m_coreInterface->CanCopy());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::Cut, m_coreInterface->CanCut());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::Paste,
		m_coreInterface->CanPaste(PasteType::Normal));
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::Properties,
		m_coreInterface->CanShowFileProperties());
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::Delete,
		browserController->IsCommandEnabled(IDM_FILE_DELETE));
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::DeletePermanently,
		browserController->IsCommandEnabled(IDM_FILE_DELETEPERMANENTLY));
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::SplitFile,
		tab.GetShellBrowserImpl()->GetNumSelectedFiles() == 1);
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::MergeFiles,
		tab.GetShellBrowserImpl()->GetNumSelectedFiles() > 1);
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::OpenCommandPrompt, !virtualFolder);
	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::NewFolder,
		m_coreInterface->CanCreate());
}

void MainToolbar::OnClipboardUpdate()
{
	if (m_browser->GetLifecycleState() != BrowserWindow::LifecycleState::Main)
	{
		return;
	}

	SendMessage(m_hwnd, TB_ENABLEBUTTON, MainToolbarButton::Paste,
		m_coreInterface->CanPaste(PasteType::Normal));
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

	auto disposition = DetermineOpenDisposition(true, WI_IsFlagSet(keysDown, MK_CONTROL),
		WI_IsFlagSet(keysDown, MK_SHIFT));
	auto *commandController = m_browser->GetCommandController();

	switch (tbButton.idCommand)
	{
	case MainToolbarButton::Back:
		commandController->ExecuteCommand(IDM_GO_BACK, disposition);
		break;

	case MainToolbarButton::Forward:
		commandController->ExecuteCommand(IDM_GO_FORWARD, disposition);
		break;

	case MainToolbarButton::Up:
		commandController->ExecuteCommand(IDM_GO_UP, disposition);
		break;
	}
}

void MainToolbar::OnTabSelected(const Tab &tab)
{
	UNREFERENCED_PARAMETER(tab);

	UpdateToolbarButtonStates();
}

void MainToolbar::OnNavigationCommitted(const NavigationRequest *request)
{
	UNREFERENCED_PARAMETER(request);

	UpdateToolbarButtonStates();
}

void MainToolbar::OnBrowserCommandTargetChanged()
{
	UpdateToolbarButtonStates();
}

void MainToolbar::OnFontOrDpiUpdated()
{
	RefreshToolbarAfterFontOrDpiChange(m_hwnd);

	sizeUpdatedSignal.m_signal();
}

MainToolbarStorage::MainToolbarButtons MainToolbar::GetButtonsForStorage() const
{
	MainToolbarStorage::MainToolbarButtons buttons;
	int numButtons = static_cast<int>(SendMessage(m_hwnd, TB_BUTTONCOUNT, 0, 0));

	for (int i = 0; i < numButtons; i++)
	{
		TBBUTTON tbButton;
		BOOL res = static_cast<BOOL>(
			SendMessage(m_hwnd, TB_GETBUTTON, i, reinterpret_cast<LPARAM>(&tbButton)));

		if (!res)
		{
			DCHECK(false);
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

		auto buttonType = MainToolbarButton::_from_integral_nothrow(id);
		CHECK(buttonType);

		buttons.AddButton(*buttonType);
	}

	return buttons;
}

void MainToolbar::StartCustomization()
{
	SendMessage(m_hwnd, TB_CUSTOMIZE, 0, 0);
}
