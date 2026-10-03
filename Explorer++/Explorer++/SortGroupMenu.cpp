// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SortGroupMenu.h"
#include "BrowserWindow.h"
#include "MainResource.h"
#include "MenuView.h"
#include "ResourceLoader.h"
#include "SelectColumnsDialog.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/SortModes.h"
#include "TabContainer.h"
#include "../Helper/SortDirection.h"

SortGroupMenu::SortGroupMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
	BrowserWindow *browser, Mode mode, const ResourceLoader *resourceLoader) :
	MenuBase(menuView, acceleratorManager),
	m_browser(browser),
	m_mode(mode),
	m_resourceLoader(resourceLoader)
{
	m_connections.push_back(
		m_rootMenuView->AddPopupWillShowObserver([this] { OnPopupWillShow(); }));
}

void SortGroupMenu::OnPopupWillShow()
{
	m_rootMenuView->ClearMenu();

	BuildMenu();
}

void SortGroupMenu::BuildMenu()
{
	auto *shellBrowser = GetActiveShellBrowser();
	auto sortModes = shellBrowser->GetAvailableSortModes();

	for (auto sortMode : sortModes)
	{
		AddSortItem(sortMode);
	}

	if (m_mode == Mode::Group && shellBrowser->GetShowInGroups())
	{
		m_rootMenuView->AppendItem(MenuItemParams(m_resourceLoader->LoadString(IDS_GROUP_BY_NONE))
				.SetKey(Item::GroupByNone)
				.SetSelectedCallback([this] { GetActiveShellBrowser()->SetShowInGroups(false); }));
	}

	m_rootMenuView->AppendSeparator();

	AddSortDirectionItem(SortDirection::Ascending);
	AddSortDirectionItem(SortDirection::Descending);

	m_rootMenuView->AppendSeparator();

	m_rootMenuView->AppendItem(MenuItemParams(m_resourceLoader->LoadString(IDS_MENU_SORT_MORE))
			.SetSelectedCallback([this] { OnSelectColumns(); }));
}

void SortGroupMenu::AddSortItem(SortMode sortMode)
{
	std::wstring helpText;

	if (auto helpTextId = MaybeGetSortModeMenuHelpStringId(sortMode);
		m_mode == Mode::Sort && helpTextId)
	{
		helpText = m_resourceLoader->LoadString(*helpTextId);
	}

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(GetSortModeMenuStringId(sortMode)))
			.SetKey(sortMode)
			.SetHelpText(helpText)
			.SetCheckedCallback([this, sortMode] { return IsSortModeActive(sortMode); })
			.SetSelectedCallback([this, sortMode] { OnSortModeSelected(sortMode); }));
}

bool SortGroupMenu::IsSortModeActive(SortMode sortMode) const
{
	auto *shellBrowser = GetActiveShellBrowser();

	if (m_mode == Mode::Sort)
	{
		return sortMode == shellBrowser->GetSortMode();
	}
	else
	{
		return shellBrowser->GetShowInGroups() && sortMode == shellBrowser->GetGroupMode();
	}
}

void SortGroupMenu::AddSortDirectionItem(SortDirection sortDirection)
{
	std::wstring helpText;

	if (m_mode == Mode::Sort)
	{
		helpText = m_resourceLoader->LoadString(GetSortDirectionMenuHelpStringId(sortDirection));
	}
	else
	{
		helpText =
			m_resourceLoader->LoadString(GetGroupSortDirectionMenuHelpStringId(sortDirection));
	}

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(GetSortDirectionMenuStringId(sortDirection)))
			.SetKey(sortDirection)
			.SetCheckStyle(MenuItemCheckStyle::Radio)
			.SetHelpText(helpText)
			.SetEnabledCallback([this] { return CanChangeSortDirection(); })
			.SetCheckedCallback(
				[this, sortDirection] { return IsSortDirectionActive(sortDirection); })
			.SetSelectedCallback(
				[this, sortDirection] { OnSortDirectionSelected(sortDirection); }));
}

UINT SortGroupMenu::GetSortDirectionMenuStringId(SortDirection sortDirection)
{
	switch (sortDirection)
	{
	case SortDirection::Ascending:
		return IDS_MENU_SORT_ASCENDING;

	case SortDirection::Descending:
		return IDS_MENU_SORT_DESCENDING;
	}

	LOG(FATAL) << "Invalid SortDirection value";
}

UINT SortGroupMenu::GetSortDirectionMenuHelpStringId(SortDirection sortDirection)
{
	switch (sortDirection)
	{
	case SortDirection::Ascending:
		return IDS_MENU_SORT_ASCENDING_HELP_TEXT;

	case SortDirection::Descending:
		return IDS_MENU_SORT_DESCENDING_HELP_TEXT;
	}

	LOG(FATAL) << "Invalid SortDirection value";
}

UINT SortGroupMenu::GetGroupSortDirectionMenuHelpStringId(SortDirection sortDirection)
{
	switch (sortDirection)
	{
	case SortDirection::Ascending:
		return IDS_MENU_GROUP_ASCENDING_HELP_TEXT;

	case SortDirection::Descending:
		return IDS_MENU_GROUP_DESCENDING_HELP_TEXT;
	}

	LOG(FATAL) << "Invalid SortDirection value";
}

bool SortGroupMenu::CanChangeSortDirection() const
{
	return m_mode == Mode::Sort || GetActiveShellBrowser()->GetShowInGroups();
}

bool SortGroupMenu::IsSortDirectionActive(SortDirection sortDirection) const
{
	auto *shellBrowser = GetActiveShellBrowser();

	if (m_mode == Mode::Sort)
	{
		return sortDirection == shellBrowser->GetSortDirection();
	}
	else
	{
		return shellBrowser->GetShowInGroups()
			&& sortDirection == shellBrowser->GetGroupSortDirection();
	}
}

void SortGroupMenu::OnSortModeSelected(SortMode sortMode)
{
	if (m_mode == Mode::Sort)
	{
		UpdateSortMode(sortMode);
	}
	else
	{
		UpdateGroupMode(sortMode);
	}
}

void SortGroupMenu::UpdateSortMode(SortMode sortMode)
{
	auto *shellBrowser = GetActiveShellBrowser();
	auto currentSortMode = shellBrowser->GetSortMode();

	if (sortMode == currentSortMode)
	{
		shellBrowser->SetSortDirection(InvertSortDirection(shellBrowser->GetSortDirection()));
	}
	else
	{
		shellBrowser->SetSortMode(sortMode);
	}
}

void SortGroupMenu::UpdateGroupMode(SortMode groupMode)
{
	auto *shellBrowser = GetActiveShellBrowser();
	auto currentGroupMode = shellBrowser->GetGroupMode();

	if (shellBrowser->GetShowInGroups() && groupMode == currentGroupMode)
	{
		shellBrowser->SetGroupSortDirection(
			InvertSortDirection(shellBrowser->GetGroupSortDirection()));
	}
	else
	{
		shellBrowser->SetGroupMode(groupMode);
		shellBrowser->SetShowInGroups(true);
	}
}

void SortGroupMenu::OnSortDirectionSelected(SortDirection sortDirection)
{
	auto *shellBrowser = GetActiveShellBrowser();

	if (m_mode == Mode::Sort)
	{
		shellBrowser->SetSortDirection(sortDirection);
	}
	else
	{
		shellBrowser->SetGroupSortDirection(sortDirection);
	}
}

void SortGroupMenu::OnSelectColumns()
{
	auto *selectColumnsDialog = SelectColumnsDialog::Create(m_resourceLoader, m_browser->GetHWND(),
		m_browser->GetActiveTabContainer()->GetSelectedTab().GetShellBrowserImpl());
	selectColumnsDialog->ShowModalDialog();
}

ShellBrowser *SortGroupMenu::GetActiveShellBrowser()
{
	return m_browser->GetActiveShellBrowser();
}

const ShellBrowser *SortGroupMenu::GetActiveShellBrowser() const
{
	return m_browser->GetActiveShellBrowser();
}
