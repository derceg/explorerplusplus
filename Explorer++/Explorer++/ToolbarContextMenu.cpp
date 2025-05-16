// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ToolbarContextMenu.h"
#include "App.h"
#include "ApplicationEditorDialog.h"
#include "Bookmarks/BookmarkClipboard.h"
#include "Bookmarks/BookmarkHelper.h"
#include "BrowserCommandController.h"
#include "BrowserWindow.h"
#include "Config.h"
#include "MainResource.h"
#include "MenuView.h"
#include "ResourceLoader.h"
#include "../Helper/ClipboardStore.h"

ToolbarContextMenu::ToolbarContextMenu(MenuView *menuView, Source source, App *app,
	BrowserWindow *browser) :
	MenuBase(menuView, app->GetAcceleratorManager()),
	m_app(app),
	m_browser(browser)
{
	BuildMenu(source, app->GetResourceLoader());

	m_connections.push_back(m_menuView->AddItemSelectedObserver(
		std::bind(&ToolbarContextMenu::OnMenuItemSelected, this, std::placeholders::_1)));
}

void ToolbarContextMenu::BuildMenu(Source source, const ResourceLoader *resourceLoader)
{
	m_menuView->AppendItem(IDM_TOOLBAR_CONTEXT_MENU_ADDRESS_BAR,
		resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_ADDRESS_BAR), {},
		resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_ADDRESS_BAR_HELP_TEXT));
	m_menuView->AppendItem(IDM_TOOLBAR_CONTEXT_MENU_MAIN_TOOLBAR,
		resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_MAIN_TOOLBAR), {},
		resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_MAIN_TOOLBAR_HELP_TEXT));
	m_menuView->AppendItem(IDM_TOOLBAR_CONTEXT_MENU_BOOKMARKS_TOOLBAR,
		resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_BOOKMARKS_TOOLBAR), {},
		resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_BOOKMARKS_TOOLBAR_HELP_TEXT));
	m_menuView->AppendItem(IDM_TOOLBAR_CONTEXT_MENU_DRIVES_TOOLBAR,
		resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_DRIVES_TOOLBAR), {},
		resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_DRIVES_TOOLBAR_HELP_TEXT));
	m_menuView->AppendItem(IDM_TOOLBAR_CONTEXT_MENU_APPLICATION_TOOLBAR,
		resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_APPLICATION_TOOLBAR), {},
		resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_APPLICATION_TOOLBAR_HELP_TEXT));
	m_menuView->AppendItem(IDM_TOOLBAR_CONTEXT_MENU_LOCK_TOOLBARS,
		resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_LOCK_TOOLBARS), {},
		resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_LOCK_TOOLBARS_HELP_TEXT));

	m_menuView->AppendSeparator();

	m_menuView->CheckItem(IDM_TOOLBAR_CONTEXT_MENU_ADDRESS_BAR,
		m_app->GetConfig()->showAddressBar.get());
	m_menuView->CheckItem(IDM_TOOLBAR_CONTEXT_MENU_MAIN_TOOLBAR,
		m_app->GetConfig()->showMainToolbar.get());
	m_menuView->CheckItem(IDM_TOOLBAR_CONTEXT_MENU_BOOKMARKS_TOOLBAR,
		m_app->GetConfig()->showBookmarksToolbar.get());
	m_menuView->CheckItem(IDM_TOOLBAR_CONTEXT_MENU_DRIVES_TOOLBAR,
		m_app->GetConfig()->showDrivesToolbar.get());
	m_menuView->CheckItem(IDM_TOOLBAR_CONTEXT_MENU_APPLICATION_TOOLBAR,
		m_app->GetConfig()->showApplicationToolbar.get());
	m_menuView->CheckItem(IDM_TOOLBAR_CONTEXT_MENU_LOCK_TOOLBARS,
		m_app->GetConfig()->lockToolbars.get());

	if (source == Source::MainToolbar)
	{
		m_menuView->AppendItem(IDM_TOOLBAR_CONTEXT_MENU_CUSTOMIZE,
			resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_CUSTOMIZE), {},
			resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_CUSTOMIZE_HELP_TEXT));
	}
	else if (source == Source::BookmarksToolbar)
	{
		m_menuView->AppendItem(IDM_TOOLBAR_CONTEXT_MENU_NEW_BOOKMARK,
			resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_NEW_BOOKMARK), {},
			resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_NEW_BOOKMARK_HELP_TEXT));
		m_menuView->AppendItem(IDM_TOOLBAR_CONTEXT_MENU_NEW_BOOKMARK_FOLDER,
			resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_NEW_BOOKMARK_FOLDER), {},
			resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_NEW_BOOKMARK_FOLDER_HELP_TEXT));
		m_menuView->AppendItem(IDM_TOOLBAR_CONTEXT_MENU_PASTE_BOOKMARK,
			resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_PASTE_BOOKMARK), {},
			resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_PASTE_BOOKMARK_HELP_TEXT));

		if (!m_app->GetClipboardStore()->IsDataAvailable(BookmarkClipboard::GetClipboardFormat()))
		{
			m_menuView->EnableItem(IDM_TOOLBAR_CONTEXT_MENU_PASTE_BOOKMARK, false);
		}
	}
	else if (source == Source::ApplicationToolbar)
	{
		m_menuView->AppendItem(IDM_TOOLBAR_CONTEXT_MENU_NEW_APPLICATION,
			resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_NEW_APPLICATION), {},
			resourceLoader->LoadString(IDS_TOOLBAR_CONTEXT_MENU_NEW_APPLICATION_HELP_TEXT));
	}

	m_menuView->RemoveTrailingSeparators();
}

void ToolbarContextMenu::OnMenuItemSelected(UINT menuItemId)
{
	auto *commandController = m_browser->GetCommandController();

	switch (menuItemId)
	{
	case IDM_TOOLBAR_CONTEXT_MENU_ADDRESS_BAR:
		commandController->ExecuteCommand(IDM_VIEW_TOOLBARS_ADDRESS_BAR);
		break;

	case IDM_TOOLBAR_CONTEXT_MENU_MAIN_TOOLBAR:
		commandController->ExecuteCommand(IDM_VIEW_TOOLBARS_MAIN_TOOLBAR);
		break;

	case IDM_TOOLBAR_CONTEXT_MENU_BOOKMARKS_TOOLBAR:
		commandController->ExecuteCommand(IDM_VIEW_TOOLBARS_BOOKMARKS_TOOLBAR);
		break;

	case IDM_TOOLBAR_CONTEXT_MENU_DRIVES_TOOLBAR:
		commandController->ExecuteCommand(IDM_VIEW_TOOLBARS_DRIVES_TOOLBAR);
		break;

	case IDM_TOOLBAR_CONTEXT_MENU_APPLICATION_TOOLBAR:
		commandController->ExecuteCommand(IDM_VIEW_TOOLBARS_APPLICATION_TOOLBAR);
		break;

	case IDM_TOOLBAR_CONTEXT_MENU_LOCK_TOOLBARS:
		commandController->ExecuteCommand(IDM_VIEW_TOOLBARS_LOCK_TOOLBARS);
		break;

	case IDM_TOOLBAR_CONTEXT_MENU_CUSTOMIZE:
		commandController->ExecuteCommand(IDM_VIEW_TOOLBARS_CUSTOMIZE);
		break;

	case IDM_TOOLBAR_CONTEXT_MENU_NEW_BOOKMARK:
		OnNewBookmarkItem(BookmarkItem::Type::Bookmark);
		break;

	case IDM_TOOLBAR_CONTEXT_MENU_NEW_BOOKMARK_FOLDER:
		OnNewBookmarkItem(BookmarkItem::Type::Folder);
		break;

	case IDM_TOOLBAR_CONTEXT_MENU_PASTE_BOOKMARK:
		OnPasteBookmark();
		break;

	case IDM_TOOLBAR_CONTEXT_MENU_NEW_APPLICATION:
		OnNewApplication();
		break;

	default:
		DCHECK(false);
		break;
	}
}

void ToolbarContextMenu::OnNewBookmarkItem(BookmarkItem::Type type)
{
	auto *bookmarkTree = m_app->GetBookmarkTree();
	BookmarkHelper::AddBookmarkItem(bookmarkTree, type, bookmarkTree->GetBookmarksToolbarFolder(),
		std::nullopt, m_browser->GetHWND(), m_browser, m_app->GetAcceleratorManager(),
		m_app->GetResourceLoader());
}

void ToolbarContextMenu::OnPasteBookmark()
{
	auto *bookmarkTree = m_app->GetBookmarkTree();
	auto *bookmarksToolbarFolder = bookmarkTree->GetBookmarksToolbarFolder();
	BookmarkHelper::PasteBookmarkItems(m_app->GetClipboardStore(), bookmarkTree,
		bookmarksToolbarFolder, bookmarksToolbarFolder->GetChildren().size());
}

void ToolbarContextMenu::OnNewApplication()
{
	using namespace Applications;

	ApplicationEditorDialog editorDialog(m_browser->GetHWND(), m_app->GetResourceLoader(),
		m_app->GetApplicationModel(),
		ApplicationEditorDialog::EditDetails::AddNewApplication(
			std::make_unique<Applications::Application>(L"", L"")));
	editorDialog.ShowModalDialog();
}
