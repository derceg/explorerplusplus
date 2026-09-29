// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ToolbarOptionsMenu.h"
#include "AppServices.h"
#include "ApplicationEditorDialog.h"
#include "Bookmarks/BookmarkClipboard.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkTree.h"
#include "BrowserWindow.h"
#include "Config.h"
#include "MainResource.h"
#include "MenuView.h"
#include "PlatformContext.h"
#include "ResourceLoader.h"
#include "../Helper/ClipboardStore.h"

ToolbarOptionsMenu::ToolbarOptionsMenu(MenuView *menuView, Source source, BrowserWindow *browser,
	AppServices *appServices) :
	MenuBase(menuView, appServices->GetAcceleratorManager()),
	m_browser(browser),
	m_appServices(appServices),
	m_config(appServices->GetConfig())
{
	BuildMenu(source, appServices->GetResourceLoader());
}

void ToolbarOptionsMenu::BuildMenu(Source source, const ResourceLoader *resourceLoader)
{
	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_ADDRESS_BAR))
			.SetHelpText(resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_ADDRESS_BAR_HELP_TEXT))
			.SetCheckedCallback([this] { return m_config->showAddressBar.get(); })
			.SetSelectedCallback(
				[this] { m_config->showAddressBar = !m_config->showAddressBar.get(); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_MAIN_TOOLBAR))
			.SetHelpText(
				resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_MAIN_TOOLBAR_HELP_TEXT))
			.SetCheckedCallback([this] { return m_config->showMainToolbar.get(); })
			.SetSelectedCallback(
				[this] { m_config->showMainToolbar = !m_config->showMainToolbar.get(); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_BOOKMARKS_TOOLBAR))
			.SetHelpText(
				resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_BOOKMARKS_TOOLBAR_HELP_TEXT))
			.SetCheckedCallback([this] { return m_config->showBookmarksToolbar.get(); })
			.SetSelectedCallback([this]
				{ m_config->showBookmarksToolbar = !m_config->showBookmarksToolbar.get(); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_DRIVES_TOOLBAR))
			.SetHelpText(
				resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_DRIVES_TOOLBAR_HELP_TEXT))
			.SetCheckedCallback([this] { return m_config->showDrivesToolbar.get(); })
			.SetSelectedCallback(
				[this] { m_config->showDrivesToolbar = !m_config->showDrivesToolbar.get(); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_APPLICATION_TOOLBAR))
			.SetHelpText(
				resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_APPLICATION_TOOLBAR_HELP_TEXT))
			.SetCheckedCallback([this] { return m_config->showApplicationToolbar.get(); })
			.SetSelectedCallback([this]
				{ m_config->showApplicationToolbar = !m_config->showApplicationToolbar.get(); }));

	m_rootMenuView->AppendSeparator();

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_LOCK_TOOLBARS))
			.SetHelpText(
				resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_LOCK_TOOLBARS_HELP_TEXT))
			.SetCheckedCallback([this] { return m_config->lockToolbars.get(); })
			.SetSelectedCallback(
				[this] { m_config->lockToolbars = !m_config->lockToolbars.get(); }));

	m_rootMenuView->AppendSeparator();

	if (source == Source::MainToolbar || source == Source::MainMenu)
	{
		m_rootMenuView->AppendItem(
			MenuItemParams(resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_CUSTOMIZE))
				.SetHelpText(
					resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_CUSTOMIZE_HELP_TEXT))
				.SetSelectedCallback([this] { m_browser->StartMainToolbarCustomization(); }));
	}
	else if (source == Source::BookmarksToolbar)
	{
		m_rootMenuView->AppendItem(
			MenuItemParams(resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_NEW_BOOKMARK))
				.SetHelpText(
					resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_NEW_BOOKMARK_HELP_TEXT))
				.SetSelectedCallback([this] { OnNewBookmarkItem(BookmarkItem::Type::Bookmark); }));

		m_rootMenuView->AppendItem(
			MenuItemParams(resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_NEW_BOOKMARK_FOLDER))
				.SetHelpText(resourceLoader->LoadString(
					IDS_TOOLBAR_CONTEXT_MENU_NEW_BOOKMARK_FOLDER_HELP_TEXT))
				.SetSelectedCallback([this] { OnNewBookmarkItem(BookmarkItem::Type::Folder); }));

		m_rootMenuView->AppendItem(
			MenuItemParams(resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_PASTE_BOOKMARK))
				.SetHelpText(
					resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_PASTE_BOOKMARK_HELP_TEXT))
				.SetEnabledCallback([this] { return CanPasteBookmark(); })
				.SetSelectedCallback([this] { OnPasteBookmark(); }));
	}
	else if (source == Source::ApplicationToolbar)
	{
		m_rootMenuView->AppendItem(
			MenuItemParams(resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_NEW_APPLICATION))
				.SetHelpText(
					resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_NEW_APPLICATION_HELP_TEXT))
				.SetSelectedCallback([this] { OnNewApplication(); }));
	}

	m_rootMenuView->RemoveTrailingSeparators();
}

bool ToolbarOptionsMenu::CanPasteBookmark() const
{
	return m_appServices->GetPlatformContext()->GetClipboardStore()->IsDataAvailable(
		BookmarkClipboard::GetClipboardFormat());
}

void ToolbarOptionsMenu::OnNewBookmarkItem(BookmarkItem::Type type)
{
	auto *bookmarkTree = m_appServices->GetBookmarkTree();
	BookmarkHelper::AddBookmarkItem(bookmarkTree, type, bookmarkTree->GetBookmarksToolbarFolder(),
		std::nullopt, m_browser->GetHWND(), m_browser, m_appServices->GetAcceleratorManager(),
		m_appServices->GetResourceLoader(), m_appServices->GetPlatformContext());
}

void ToolbarOptionsMenu::OnPasteBookmark()
{
	auto *bookmarkTree = m_appServices->GetBookmarkTree();
	auto *bookmarksToolbarFolder = bookmarkTree->GetBookmarksToolbarFolder();
	BookmarkHelper::PasteBookmarkItems(m_appServices->GetPlatformContext()->GetClipboardStore(),
		bookmarkTree, bookmarksToolbarFolder, bookmarksToolbarFolder->GetChildren().size());
}

void ToolbarOptionsMenu::OnNewApplication()
{
	using namespace Applications;

	auto *editorDialog = ApplicationEditorDialog::Create(m_browser->GetHWND(),
		m_appServices->GetResourceLoader(), m_appServices->GetApplicationModel(),
		ApplicationEditorDialog::EditDetails::AddNewApplication(
			std::make_unique<Applications::Application>(L"", L"")));
	editorDialog->ShowModalDialog();
}
