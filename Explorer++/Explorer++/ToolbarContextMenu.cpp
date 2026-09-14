// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ToolbarContextMenu.h"
#include "AppServices.h"
#include "ApplicationEditorDialog.h"
#include "Bookmarks/BookmarkClipboard.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkTree.h"
#include "BrowserCommandController.h"
#include "BrowserWindow.h"
#include "Config.h"
#include "MainResource.h"
#include "MenuView.h"
#include "PlatformContext.h"
#include "ResourceLoader.h"
#include "../Helper/ClipboardStore.h"

ToolbarContextMenu::ToolbarContextMenu(MenuView *menuView, Source source, BrowserWindow *browser,
	AppServices *appServices) :
	MenuBase(menuView, appServices->GetAcceleratorManager()),
	m_browser(browser),
	m_appServices(appServices)
{
	m_menuView->SetDelegate(this);

	BuildMenu(source, appServices->GetResourceLoader());
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

	const auto *config = m_appServices->GetConfig();
	m_menuView->CheckItem(IDM_TOOLBAR_CONTEXT_MENU_ADDRESS_BAR, config->showAddressBar.get());
	m_menuView->CheckItem(IDM_TOOLBAR_CONTEXT_MENU_MAIN_TOOLBAR, config->showMainToolbar.get());
	m_menuView->CheckItem(IDM_TOOLBAR_CONTEXT_MENU_BOOKMARKS_TOOLBAR,
		config->showBookmarksToolbar.get());
	m_menuView->CheckItem(IDM_TOOLBAR_CONTEXT_MENU_DRIVES_TOOLBAR, config->showDrivesToolbar.get());
	m_menuView->CheckItem(IDM_TOOLBAR_CONTEXT_MENU_APPLICATION_TOOLBAR,
		config->showApplicationToolbar.get());
	m_menuView->CheckItem(IDM_TOOLBAR_CONTEXT_MENU_LOCK_TOOLBARS, config->lockToolbars.get());

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

		if (!m_appServices->GetPlatformContext()->GetClipboardStore()->IsDataAvailable(
				BookmarkClipboard::GetClipboardFormat()))
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

void ToolbarContextMenu::OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	UNREFERENCED_PARAMETER(isCtrlKeyDown);
	UNREFERENCED_PARAMETER(isShiftKeyDown);

	auto *commandController = m_browser->GetCommandController();

	switch (id)
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
	auto *bookmarkTree = m_appServices->GetBookmarkTree();
	BookmarkHelper::AddBookmarkItem(bookmarkTree, type, bookmarkTree->GetBookmarksToolbarFolder(),
		std::nullopt, m_browser->GetHWND(), m_browser, m_appServices->GetAcceleratorManager(),
		m_appServices->GetResourceLoader(), m_appServices->GetPlatformContext());
}

void ToolbarContextMenu::OnPasteBookmark()
{
	auto *bookmarkTree = m_appServices->GetBookmarkTree();
	auto *bookmarksToolbarFolder = bookmarkTree->GetBookmarksToolbarFolder();
	BookmarkHelper::PasteBookmarkItems(m_appServices->GetPlatformContext()->GetClipboardStore(),
		bookmarkTree, bookmarksToolbarFolder, bookmarksToolbarFolder->GetChildren().size());
}

void ToolbarContextMenu::OnNewApplication()
{
	using namespace Applications;

	auto *editorDialog = ApplicationEditorDialog::Create(m_browser->GetHWND(),
		m_appServices->GetResourceLoader(), m_appServices->GetApplicationModel(),
		ApplicationEditorDialog::EditDetails::AddNewApplication(
			std::make_unique<Applications::Application>(L"", L"")));
	editorDialog->ShowModalDialog();
}
