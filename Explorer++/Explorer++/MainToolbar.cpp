// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainToolbar.h"
#include "AppServices.h"
#include "BrowserCommandController.h"
#include "Config.h"
#include "MainResource.h"
#include "MainToolbarLayout.h"
#include "MainToolbarView.h"
#include "NavigationHelper.h"
#include "PopupMenuView.h"
#include "ResourceLoader.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "TabEvents.h"
#include "TabHistoryMenu.h"
#include "TabParentItemsMenu.h"
#include "TestHelper.h"
#include "ViewsMenu.h"
#include "../Helper/ClipboardWatcher.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/EnumHelper.h"
#include <fmt/format.h>
#include <fmt/xchar.h>
#include <magic_enum/magic_enum.hpp>
#include <algorithm>

namespace
{

const int TOOLBAR_IMAGE_SIZE_SMALL = 16;
const int TOOLBAR_IMAGE_SIZE_LARGE = 24;

constexpr MainToolbarButtonId DEFAULT_TOOLBAR_BUTTONS[] = {
	MainToolbarButtonId::Back,
	MainToolbarButtonId::Forward,
	MainToolbarButtonId::Up,
	MainToolbarButtonId::Separator,
	MainToolbarButtonId::Folders,
	MainToolbarButtonId::Separator,
	MainToolbarButtonId::Cut,
	MainToolbarButtonId::Copy,
	MainToolbarButtonId::Paste,
	MainToolbarButtonId::Delete,
	MainToolbarButtonId::DeletePermanently,
	MainToolbarButtonId::Properties,
	MainToolbarButtonId::Search,
	MainToolbarButtonId::Separator,
	MainToolbarButtonId::NewFolder,
	MainToolbarButtonId::CopyTo,
	MainToolbarButtonId::MoveTo,
	MainToolbarButtonId::Separator,
	MainToolbarButtonId::Views,
	MainToolbarButtonId::OpenCommandPrompt,
	MainToolbarButtonId::Refresh,
	MainToolbarButtonId::Separator,
	MainToolbarButtonId::AddBookmark,
	MainToolbarButtonId::Bookmarks,
};

struct ButtonIconMapping
{
	MainToolbarButtonId button;
	Icon icon;
};

constexpr auto BUTTON_ICON_MAPPINGS = std::to_array<ButtonIconMapping>({
	{ MainToolbarButtonId::Back, Icon::Back },
	{ MainToolbarButtonId::Forward, Icon::Forward },
	{ MainToolbarButtonId::Up, Icon::Up },
	{ MainToolbarButtonId::Folders, Icon::FolderTree },
	{ MainToolbarButtonId::CopyTo, Icon::CopyTo },
	{ MainToolbarButtonId::MoveTo, Icon::MoveTo },
	{ MainToolbarButtonId::NewFolder, Icon::NewFolder },
	{ MainToolbarButtonId::Copy, Icon::Copy },
	{ MainToolbarButtonId::Cut, Icon::Cut },
	{ MainToolbarButtonId::Paste, Icon::Paste },
	{ MainToolbarButtonId::Delete, Icon::Delete },
	{ MainToolbarButtonId::Views, Icon::Views },
	{ MainToolbarButtonId::Search, Icon::Search },
	{ MainToolbarButtonId::Properties, Icon::Properties },
	{ MainToolbarButtonId::Refresh, Icon::Refresh },
	{ MainToolbarButtonId::AddBookmark, Icon::AddBookmark },
	{ MainToolbarButtonId::NewTab, Icon::NewTab },
	{ MainToolbarButtonId::OpenCommandPrompt, Icon::CommandLine },
	{ MainToolbarButtonId::Bookmarks, Icon::Bookmarks },
	{ MainToolbarButtonId::DeletePermanently, Icon::DeletePermanently },
	{ MainToolbarButtonId::SplitFile, Icon::SplitFiles },
	{ MainToolbarButtonId::MergeFiles, Icon::MergeFiles },
	{ MainToolbarButtonId::CloseTab, Icon::CloseTab },
});

// There should be an icon for each button. Note that the MainToolbarButtonId enum contains one
// additional item - for the separator.
static_assert(BUTTON_ICON_MAPPINGS.size() == (magic_enum::enum_count<MainToolbarButtonId>() - 1));

constexpr bool VerifyButtonIconMappings(std::span<const ButtonIconMapping> mappings)
{
	for (auto buttonId : magic_enum::enum_values<MainToolbarButtonId>())
	{
		auto expectedCount = (buttonId == MainToolbarButtonId::Separator) ? 0 : 1;

		if (std::ranges::count(mappings, buttonId, &ButtonIconMapping::button) != expectedCount)
		{
			return false;
		}
	}

	return true;
}

static_assert(VerifyButtonIconMappings(BUTTON_ICON_MAPPINGS));

std::wstring GetButtonText(MainToolbarButtonId buttonId, const ResourceLoader *resourceLoader);

class MainToolbarButton : public ToolbarButton
{
public:
	MainToolbarButton(MainToolbarButtonId buttonId, int imageIndex, BrowserWindow *browser,
		const ResourceLoader *resourceLoader) :
		ToolbarButton(std::bind(&MainToolbarButton::OnClicked, this)),
		m_buttonId(buttonId),
		m_browser(browser),
		m_resourceLoader(resourceLoader),
		m_imageIndex(imageIndex)
	{
	}

	std::wstring GetText() const override
	{
		return GetButtonText(m_buttonId, m_resourceLoader);
	}

	bool ShouldShowText() const override
	{
		// By default, main toolbar buttons will only display an icon.
		return false;
	}

	std::wstring GetTooltipText() const override
	{
		// This will result in the button text being shown as the tooltip.
		return {};
	}

	std::optional<int> GetImageIndex() const override
	{
		return m_imageIndex;
	}

	bool IsEnabled() const override
	{
		return m_enabled;
	}

	void SetEnabled(bool enabled)
	{
		if (enabled == m_enabled)
		{
			return;
		}

		m_enabled = enabled;

		NotifyParentOfUpdate();
	}

protected:
	const MainToolbarButtonId m_buttonId;
	BrowserWindow *const m_browser;
	const ResourceLoader *const m_resourceLoader;

private:
	void OnClicked()
	{
		SendMessage(m_browser->GetHWND(), WM_COMMAND, MAKEWPARAM(ToUnderlying(m_buttonId), 0), 0);
	}

	bool m_enabled = true;
	const int m_imageIndex;
};

class MainToolbarHistoryButton : public MainToolbarButton
{
public:
	enum HistoryType
	{
		Back,
		Forward
	};

	MainToolbarHistoryButton(HistoryType historyType, int imageIndex, BrowserWindow *browser,
		const AcceleratorManager *acceleratorManager, ShellIconLoader *shellIconLoader,
		const ResourceLoader *resourceLoader) :
		MainToolbarButton(historyType == HistoryType::Back ? MainToolbarButtonId::Back
														   : MainToolbarButtonId::Forward,
			imageIndex, browser, resourceLoader),
		m_historyType(historyType),
		m_acceleratorManager(acceleratorManager),
		m_shellIconLoader(shellIconLoader)
	{
		SetMiddleClickedCallback(std::bind_front(&MainToolbarHistoryButton::OnMiddleClicked, this));
	}

	std::wstring GetTooltipText() const override
	{
		const auto *shellBrowser = m_browser->GetActiveShellBrowser();
		auto *entry = shellBrowser->GetNavigationController()->GetEntry(
			m_historyType == HistoryType::Back ? -1 : 1);

		if (!entry)
		{
			return {};
		}

		std::wstring infoTipTemplate = m_resourceLoader->LoadString(
			m_historyType == HistoryType::Back ? IDS_MAIN_TOOLBAR_BACK : IDS_MAIN_TOOLBAR_FORWARD);
		return fmt::format(fmt::runtime(infoTipTemplate),
			fmt::arg(L"folder_name",
				GetDisplayNameWithFallback(entry->GetPidl().Raw(), SHGDN_INFOLDER)));
	}

	bool IsDropDownButton() const override
	{
		return true;
	}

	MenuFactory MaybeGetMenuFactory() const override
	{
		const auto *shellBrowser = m_browser->GetActiveShellBrowser();
		const auto *navigationController = shellBrowser->GetNavigationController();

		if ((m_historyType == HistoryType::Back && !navigationController->CanGoBack())
			|| (m_historyType == HistoryType::Forward && !navigationController->CanGoForward()))
		{
			return {};
		}

		return [this, historyType = m_historyType](MenuView *menuView)
		{
			return std::make_unique<TabHistoryMenu>(menuView, m_acceleratorManager, m_browser,
				m_shellIconLoader,
				historyType == HistoryType::Back ? TabHistoryMenu::MenuType::Back
												 : TabHistoryMenu::MenuType::Forward);
		};
	}

private:
	void OnMiddleClicked(const MouseEvent &event)
	{
		int command;

		if (m_historyType == HistoryType::Back)
		{
			command = IDM_GO_BACK;
		}
		else
		{
			command = IDM_GO_FORWARD;
		}

		auto disposition = DetermineOpenDisposition(true, event.ctrlKey, event.shiftKey);
		m_browser->GetCommandController()->ExecuteCommand(command, disposition);
	}

	const HistoryType m_historyType;
	const AcceleratorManager *const m_acceleratorManager;
	ShellIconLoader *const m_shellIconLoader;
};

class MainToolbarUpButton : public MainToolbarButton
{
public:
	MainToolbarUpButton(int imageIndex, BrowserWindow *browser,
		const AcceleratorManager *acceleratorManager, ShellIconLoader *shellIconLoader,
		const ResourceLoader *resourceLoader) :
		MainToolbarButton(MainToolbarButtonId::Up, imageIndex, browser, resourceLoader),
		m_acceleratorManager(acceleratorManager),
		m_shellIconLoader(shellIconLoader)
	{
		SetMiddleClickedCallback(std::bind_front(&MainToolbarUpButton::OnMiddleClicked, this));
	}

	// If there's a parent folder to navigate up to, a customized infotip will be generated that
	// contains the name of the folder.
	std::wstring GetTooltipText() const override
	{
		const auto *shellBrowser = m_browser->GetActiveShellBrowser();
		const auto *currentEntry = shellBrowser->GetNavigationController()->GetCurrentEntry();

		unique_pidl_absolute parentPidl;
		HRESULT hr =
			GetVirtualParentPath(currentEntry->GetPidl().Raw(), wil::out_param(parentPidl));

		if (FAILED(hr))
		{
			return {};
		}

		std::wstring parentName;
		hr = GetDisplayName(parentPidl.get(), SHGDN_NORMAL, parentName);

		if (FAILED(hr))
		{
			return {};
		}

		std::wstring infoTipTemplate = m_resourceLoader->LoadString(IDS_MAIN_TOOLBAR_UP_TO_FOLDER);
		std::wstring infoTip =
			fmt::format(fmt::runtime(infoTipTemplate), fmt::arg(L"folder_name", parentName));

		return infoTip;
	}

	bool IsDropDownButton() const override
	{
		return true;
	}

	MenuFactory MaybeGetMenuFactory() const override
	{
		if (!m_browser->GetActiveShellBrowser()->GetNavigationController()->CanGoUp())
		{
			return {};
		}

		return [this](MenuView *menuView)
		{
			return std::make_unique<TabParentItemsMenu>(menuView, m_acceleratorManager, m_browser,
				m_shellIconLoader);
		};
	}

private:
	void OnMiddleClicked(const MouseEvent &event)
	{
		auto disposition = DetermineOpenDisposition(true, event.ctrlKey, event.shiftKey);
		m_browser->GetCommandController()->ExecuteCommand(IDM_GO_UP, disposition);
	}

	const AcceleratorManager *const m_acceleratorManager;
	ShellIconLoader *const m_shellIconLoader;
};

class MainToolbarFoldersButton : public MainToolbarButton
{
public:
	MainToolbarFoldersButton(int imageIndex, BrowserWindow *browser, const Config *config,
		const ResourceLoader *resourceLoader) :
		MainToolbarButton(MainToolbarButtonId::Folders, imageIndex, browser, resourceLoader),
		m_config(config)
	{
		m_connections.push_back(m_config->showFolders.addObserver(
			std::bind(&MainToolbarFoldersButton::OnShowFoldersUpdated, this)));
	}

	bool ShouldShowText() const override
	{
		return true;
	}

	bool IsChecked() const override
	{
		return m_config->showFolders.get();
	}

private:
	void OnShowFoldersUpdated()
	{
		NotifyParentOfUpdate();
	}

	const Config *const m_config;
	std::vector<boost::signals2::scoped_connection> m_connections;
};

class MainToolbarViewsButton : public MainToolbarButton
{
public:
	MainToolbarViewsButton(int imageIndex, BrowserWindow *browser,
		const AcceleratorManager *acceleratorManager, const ResourceLoader *resourceLoader) :
		MainToolbarButton(MainToolbarButtonId::Views, imageIndex, browser, resourceLoader),
		m_acceleratorManager(acceleratorManager)
	{
	}

	bool IsDropDownButton() const override
	{
		return true;
	}

	MenuFactory MaybeGetMenuFactory() const override
	{
		return [this](MenuView *menuView)
		{
			return std::make_unique<ViewsMenu>(menuView, m_acceleratorManager, m_browser,
				m_resourceLoader);
		};
	}

private:
	const AcceleratorManager *const m_acceleratorManager;
};

}

MainToolbar::MainToolbar(MainToolbarView *view,
	const std::optional<MainToolbarLayout> &initialLayout, BrowserWindow *browser,
	AppServices *appServices, CoreInterface *coreInterface, ShellIconLoader *shellIconLoader) :
	ToolbarBase(view, browser, appServices->GetAcceleratorManager()),
	m_view(view),
	m_browser(browser),
	m_appServices(appServices),
	m_coreInterface(coreInterface),
	m_shellIconLoader(shellIconLoader),
	m_config(appServices->GetConfig()),
	m_resourceLoader(appServices->GetResourceLoader())
{
	Initialize(initialLayout);
}

void MainToolbar::Initialize(const std::optional<MainToolbarLayout> &initialLayout)
{
	CreateImageLists();
	SetToolbarImageList();

	InitializeAvailableButtons();
	m_view->SetDefaultItems(ToToolbarItems(GetDefaultButtons()));
	m_view->SetItems(
		ToToolbarItems(initialLayout ? initialLayout->GetButtons() : GetDefaultButtons()));

	m_connections.push_back(m_browser->AddLifecycleStateChangedObserver(
		std::bind_front(&MainToolbar::OnBrowserLifecycleStateChanged, this)));

	m_connections.push_back(m_appServices->GetTabEvents()->AddSelectedObserver(
		std::bind_front(&MainToolbar::OnTabSelected, this), TabEventScope::ForBrowser(*m_browser)));

	m_connections.push_back(m_appServices->GetNavigationEvents()->AddCommittedObserver(
		std::bind_front(&MainToolbar::OnNavigationCommitted, this),
		NavigationEventScope::ForActiveShellBrowser(*m_browser)));

	m_connections.push_back(m_browser->GetCommandTargetManager()->targetChangedSignal.AddObserver(
		std::bind_front(&MainToolbar::OnBrowserCommandTargetChanged, this)));
	m_connections.push_back(m_config->useLargeToolbarIcons.addObserver(
		std::bind_front(&MainToolbar::OnUseLargeToolbarIconsUpdated, this)));

	m_connections.push_back(m_appServices->GetClipboardWatcher()->AddClipboardUpdatedObserver(
		std::bind_front(&MainToolbar::OnClipboardUpdate, this)));
}

void MainToolbar::CreateImageLists()
{
	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(m_view->GetHWND());

	int dpiScaledSizeSmall = MulDiv(TOOLBAR_IMAGE_SIZE_SMALL, dpi, USER_DEFAULT_SCREEN_DPI);
	int dpiScaledSizeLarge = MulDiv(TOOLBAR_IMAGE_SIZE_LARGE, dpi, USER_DEFAULT_SCREEN_DPI);

	int numImages = static_cast<int>(magic_enum::enum_count<MainToolbarButtonId>() - 1);

	m_imageListSmall.reset(ImageList_Create(dpiScaledSizeSmall, dpiScaledSizeSmall,
		ILC_COLOR32 | ILC_MASK, numImages, 0));
	m_imageListLarge.reset(ImageList_Create(dpiScaledSizeLarge, dpiScaledSizeLarge,
		ILC_COLOR32 | ILC_MASK, numImages, 0));

	for (const auto &mapping : BUTTON_ICON_MAPPINGS)
	{
		wil::unique_hbitmap smallBitmap = m_resourceLoader->LoadBitmapFromPNGForDpi(mapping.icon,
			TOOLBAR_IMAGE_SIZE_SMALL, TOOLBAR_IMAGE_SIZE_SMALL, dpi);
		int smallImageIndex = ImageList_Add(m_imageListSmall.get(), smallBitmap.get(), nullptr);
		CHECK(smallImageIndex != -1);

		wil::unique_hbitmap largeBitmap = m_resourceLoader->LoadBitmapFromPNGForDpi(mapping.icon,
			TOOLBAR_IMAGE_SIZE_LARGE, TOOLBAR_IMAGE_SIZE_LARGE, dpi);
		int largeImageIndex = ImageList_Add(m_imageListLarge.get(), largeBitmap.get(), nullptr);
		CHECK(largeImageIndex != -1);

		// When the config option to use large toolbar icons is toggled, the image list assigned to
		// the toolbar control will be changed. The image indexes for the buttons won't be updated,
		// so it's important that the image indexes are the same for both the small/large image
		// lists. That should be the case, since the image lists are created with the same set of
		// images.
		CHECK(largeImageIndex == smallImageIndex);

		m_idToImageIndexMap.insert({ mapping.button, smallImageIndex });
	}
}

void MainToolbar::SetToolbarImageList()
{
	HIMAGELIST imageList;

	if (m_config->useLargeToolbarIcons.get())
	{
		imageList = m_imageListLarge.get();
	}
	else
	{
		imageList = m_imageListSmall.get();
	}

	m_view->SetImageList(imageList);
}

void MainToolbar::InitializeAvailableButtons()
{
	std::vector<std::unique_ptr<ToolbarButton>> buttons;

	for (auto buttonId : magic_enum::enum_values<MainToolbarButtonId>())
	{
		if (buttonId == MainToolbarButtonId::Separator)
		{
			continue;
		}

		auto button = CreateButtonForId(buttonId);
		m_idToButtonMap.insert({ buttonId, button.get() });

		buttons.push_back(std::move(button));
	}

	m_view->InitializeAvailableButtons(std::move(buttons));
}

std::unique_ptr<ToolbarButton> MainToolbar::CreateButtonForId(MainToolbarButtonId buttonId)
{
	auto itr = m_idToImageIndexMap.find(buttonId);
	CHECK(itr != m_idToImageIndexMap.end());
	int imageIndex = itr->second;

	switch (buttonId)
	{
	// MainToolbarButtonId::Separator is a valid ID, but there shouldn't be an attempt to create a
	// button using this ID, as separators and buttons are managed separately. That is, the
	// available set of buttons includes only buttons and separators are added via an independent
	// mechanism, not via ToolbarButton.
	case MainToolbarButtonId::Separator:
		CHECK(false);
		return nullptr;

	case MainToolbarButtonId::Back:
		return std::make_unique<MainToolbarHistoryButton>(
			MainToolbarHistoryButton::HistoryType::Back, imageIndex, m_browser,
			m_appServices->GetAcceleratorManager(), m_shellIconLoader, m_resourceLoader);

	case MainToolbarButtonId::Forward:
		return std::make_unique<MainToolbarHistoryButton>(
			MainToolbarHistoryButton::HistoryType::Forward, imageIndex, m_browser,
			m_appServices->GetAcceleratorManager(), m_shellIconLoader, m_resourceLoader);

	case MainToolbarButtonId::Up:
		return std::make_unique<MainToolbarUpButton>(imageIndex, m_browser,
			m_appServices->GetAcceleratorManager(), m_shellIconLoader, m_resourceLoader);

	case MainToolbarButtonId::Folders:
		return std::make_unique<MainToolbarFoldersButton>(imageIndex, m_browser, m_config,
			m_resourceLoader);

	case MainToolbarButtonId::Views:
		return std::make_unique<MainToolbarViewsButton>(imageIndex, m_browser,
			m_appServices->GetAcceleratorManager(), m_resourceLoader);

	default:
		return std::make_unique<MainToolbarButton>(buttonId, imageIndex, m_browser,
			m_resourceLoader);
	}
}

std::vector<ToolbarItem> MainToolbar::ToToolbarItems(
	const std::vector<MainToolbarButtonId> &buttonIds)
{
	std::vector<ToolbarItem> items;

	for (auto buttonId : buttonIds)
	{
		if (buttonId == MainToolbarButtonId::Separator)
		{
			items.push_back(ToolbarItem::Separator());
		}
		else
		{
			auto *button = GetButtonForId(buttonId);
			items.push_back(ToolbarItem::Button(button));
		}
	}

	return items;
}

std::vector<MainToolbarButtonId> MainToolbar::GetDefaultButtons() const
{
	return { std::begin(DEFAULT_TOOLBAR_BUTTONS), std::end(DEFAULT_TOOLBAR_BUTTONS) };
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

	SetToolbarImageList();
}

void MainToolbar::UpdateToolbarButtonStates()
{
	if (m_browser->GetLifecycleState() != BrowserWindow::LifecycleState::Main)
	{
		return;
	}

	auto *browserController = m_browser->GetCommandController();

	EnableButton(MainToolbarButtonId::Back, browserController->IsCommandEnabled(IDM_GO_BACK));
	EnableButton(MainToolbarButtonId::Forward, browserController->IsCommandEnabled(IDM_GO_FORWARD));
	EnableButton(MainToolbarButtonId::Up, browserController->IsCommandEnabled(IDM_GO_UP));
	EnableButton(MainToolbarButtonId::Cut, browserController->IsCommandEnabled(IDM_EDIT_CUT));
	EnableButton(MainToolbarButtonId::Copy, browserController->IsCommandEnabled(IDM_EDIT_COPY));
	EnableButton(MainToolbarButtonId::MoveTo,
		browserController->IsCommandEnabled(IDM_EDIT_MOVETOFOLDER));
	EnableButton(MainToolbarButtonId::CopyTo,
		browserController->IsCommandEnabled(IDM_EDIT_COPYTOFOLDER));
	EnableButton(MainToolbarButtonId::Paste, m_coreInterface->CanPaste(PasteType::Normal));
	EnableButton(MainToolbarButtonId::Properties,
		browserController->IsCommandEnabled(IDM_FILE_PROPERTIES));
	EnableButton(MainToolbarButtonId::Delete, browserController->IsCommandEnabled(IDM_FILE_DELETE));
	EnableButton(MainToolbarButtonId::DeletePermanently,
		browserController->IsCommandEnabled(IDM_FILE_DELETEPERMANENTLY));
	EnableButton(MainToolbarButtonId::SplitFile,
		browserController->IsCommandEnabled(IDM_ACTIONS_SPLITFILE));
	EnableButton(MainToolbarButtonId::MergeFiles,
		browserController->IsCommandEnabled(IDM_ACTIONS_MERGEFILES));
	EnableButton(MainToolbarButtonId::OpenCommandPrompt,
		browserController->IsCommandEnabled(IDM_FILE_OPENCOMMANDPROMPT));
	EnableButton(MainToolbarButtonId::NewFolder,
		browserController->IsCommandEnabled(IDM_ACTIONS_NEWFOLDER));
}

void MainToolbar::OnClipboardUpdate()
{
	if (m_browser->GetLifecycleState() != BrowserWindow::LifecycleState::Main)
	{
		return;
	}

	EnableButton(MainToolbarButtonId::Paste, m_coreInterface->CanPaste(PasteType::Normal));
}

void MainToolbar::EnableButton(MainToolbarButtonId buttonId, bool enable)
{
	auto *button = static_cast<MainToolbarButton *>(GetButtonForId(buttonId));
	button->SetEnabled(enable);
}

MainToolbarLayout MainToolbar::GetLayoutForStorage() const
{
	MainToolbarLayout layout;

	for (const auto &item : m_view->GetItems())
	{
		if (item.IsButton())
		{
			layout.AddButton(GetIdForButton(item.GetButton()));
		}
		else
		{
			layout.AddButton(MainToolbarButtonId::Separator);
		}
	}

	return layout;
}

ToolbarButton *MainToolbar::GetButtonForId(MainToolbarButtonId buttonId) const
{
	auto itr = m_idToButtonMap.left.find(buttonId);
	CHECK(itr != m_idToButtonMap.left.end());
	return itr->second;
}

MainToolbarButtonId MainToolbar::GetIdForButton(ToolbarButton *button) const
{
	auto itr = m_idToButtonMap.right.find(button);
	CHECK(itr != m_idToButtonMap.right.end());
	return itr->second;
}

ToolbarButton *MainToolbar::GetButtonForIdForTesting(MainToolbarButtonId buttonId) const
{
	CHECK(IsInTest());
	return GetButtonForId(buttonId);
}

namespace
{

std::wstring GetButtonText(MainToolbarButtonId buttonId, const ResourceLoader *resourceLoader)
{
	std::optional<UINT> stringId;

	switch (buttonId)
	{
	case MainToolbarButtonId::Separator:
		// A separator has no text and there shouldn't be any attempt to retrieve the text in this
		// case.
		CHECK(false);
		break;

	case MainToolbarButtonId::Back:
		stringId = IDS_TOOLBAR_BACK;
		break;

	case MainToolbarButtonId::Forward:
		stringId = IDS_TOOLBAR_FORWARD;
		break;

	case MainToolbarButtonId::Up:
		stringId = IDS_TOOLBAR_UP;
		break;

	case MainToolbarButtonId::Folders:
		stringId = IDS_TOOLBAR_FOLDERS;
		break;

	case MainToolbarButtonId::CopyTo:
		stringId = IDS_TOOLBAR_COPYTO;
		break;

	case MainToolbarButtonId::MoveTo:
		stringId = IDS_TOOLBAR_MOVETO;
		break;

	case MainToolbarButtonId::NewFolder:
		stringId = IDS_TOOLBAR_NEWFOLDER;
		break;

	case MainToolbarButtonId::Copy:
		stringId = IDS_TOOLBAR_COPY;
		break;

	case MainToolbarButtonId::Cut:
		stringId = IDS_TOOLBAR_CUT;
		break;

	case MainToolbarButtonId::Paste:
		stringId = IDS_TOOLBAR_PASTE;
		break;

	case MainToolbarButtonId::Delete:
		stringId = IDS_TOOLBAR_DELETE;
		break;

	case MainToolbarButtonId::DeletePermanently:
		stringId = IDS_TOOLBAR_DELETEPERMANENTLY;
		break;

	case MainToolbarButtonId::Views:
		stringId = IDS_TOOLBAR_VIEWS;
		break;

	case MainToolbarButtonId::Search:
		stringId = IDS_TOOLBAR_SEARCH;
		break;

	case MainToolbarButtonId::Properties:
		stringId = IDS_TOOLBAR_PROPERTIES;
		break;

	case MainToolbarButtonId::Refresh:
		stringId = IDS_TOOLBAR_REFRESH;
		break;

	case MainToolbarButtonId::AddBookmark:
		stringId = IDS_TOOLBAR_ADDBOOKMARK;
		break;

	case MainToolbarButtonId::Bookmarks:
		stringId = IDS_TOOLBAR_MANAGEBOOKMARKS;
		break;

	case MainToolbarButtonId::NewTab:
		stringId = IDS_TOOLBAR_NEWTAB;
		break;

	case MainToolbarButtonId::OpenCommandPrompt:
		stringId = IDS_TOOLBAR_OPENCOMMANDPROMPT;
		break;

	case MainToolbarButtonId::SplitFile:
		stringId = IDS_TOOLBAR_SPLIT_FILE;
		break;

	case MainToolbarButtonId::MergeFiles:
		stringId = IDS_TOOLBAR_MERGE_FILES;
		break;

	case MainToolbarButtonId::CloseTab:
		stringId = IDS_TOOLBAR_CLOSE_TAB;
		break;
	}

	CHECK(stringId);
	return resourceLoader->LoadString(*stringId);
}

}
