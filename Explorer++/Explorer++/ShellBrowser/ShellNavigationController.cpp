// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellNavigationController.h"
#include "ShellNavigator.h"
#include "TabNavigationInterface.h"
#include "../Helper/IconFetcher.h"

ShellNavigationController::ShellNavigationController(ShellNavigator *navigator,
	TabNavigationInterface *tabNavigation, IconFetcherInterface *iconFetcher) :
	m_navigator(navigator),
	m_tabNavigation(tabNavigation),
	m_iconFetcher(iconFetcher)
{
	Initialize();
}

ShellNavigationController::ShellNavigationController(ShellNavigator *navigator,
	TabNavigationInterface *tabNavigation, IconFetcherInterface *iconFetcher,
	const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries, int currentEntry) :
	NavigationController(CopyPreservedHistoryEntries(preservedEntries), currentEntry),
	m_navigator(navigator),
	m_tabNavigation(tabNavigation),
	m_iconFetcher(iconFetcher)
{
	Initialize();
}

void ShellNavigationController::Initialize()
{
	m_connections.emplace_back(m_navigator->AddNavigationCommittedObserver(
		std::bind_front(&ShellNavigationController::OnNavigationCommitted, this),
		boost::signals2::at_front));
}

std::vector<std::unique_ptr<HistoryEntry>> ShellNavigationController::CopyPreservedHistoryEntries(
	const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries)
{
	std::vector<std::unique_ptr<HistoryEntry>> entries;

	for (const auto &preservedEntry : preservedEntries)
	{
		auto entry = std::make_unique<HistoryEntry>(*preservedEntry);
		entries.push_back(std::move(entry));
	}

	return entries;
}

void ShellNavigationController::OnNavigationCommitted(const NavigateParams &navigateParams)
{
	if (navigateParams.historyEntryType == HistoryEntryType::AddEntry
		|| navigateParams.historyEntryType == HistoryEntryType::ReplaceCurrentEntry)
	{
		auto entry = BuildEntry(navigateParams);
		int entryId = entry->GetId();
		int entryIndex;

		if (navigateParams.historyEntryType == HistoryEntryType::AddEntry)
		{
			entryIndex = AddEntry(std::move(entry));
		}
		else
		{
			entryIndex = ReplaceCurrentEntry(std::move(entry));
		}

		// TODO: It would probably be better to do this somewhere else, since
		// this class is focused on navigation.
		m_iconFetcher->QueueIconTask(navigateParams.pidl.Raw(),
			[this, entryIndex, entryId](int iconIndex)
			{
				auto *entry = GetEntryAtIndex(entryIndex);

				if (!entry || entry->GetId() != entryId)
				{
					return;
				}

				entry->SetSystemIconIndex(iconIndex);
			});
	}

	if (navigateParams.historyEntryId)
	{
		auto *entry = GetEntryById(*navigateParams.historyEntryId);

		if (entry)
		{
			auto index = GetIndexOfEntry(entry);
			SetCurrentIndex(*index);
		}
	}
}

std::unique_ptr<HistoryEntry> ShellNavigationController::BuildEntry(
	const NavigateParams &navigateParams)
{
	std::wstring displayName;
	HRESULT hr = GetDisplayName(navigateParams.pidl.Raw(), SHGDN_INFOLDER, displayName);

	if (FAILED(hr))
	{
		// It's not expected that this would happen, so it would be useful to have some
		// indication if the call above ever does fail.
		assert(false);

		displayName = L"(Unknown)";
	}

	auto fullPathForDisplay = GetFolderPathForDisplay(navigateParams.pidl.Raw());

	if (!fullPathForDisplay)
	{
		assert(false);

		fullPathForDisplay = L"(Unknown)";
	}

	return std::make_unique<HistoryEntry>(navigateParams.pidl.Raw(), displayName,
		*fullPathForDisplay);
}

HRESULT ShellNavigationController::GoToOffset(int offset)
{
	auto entry = GetEntry(offset);

	if (!entry)
	{
		return E_FAIL;
	}

	return Navigate(entry);
}

bool ShellNavigationController::CanGoUp() const
{
	auto *currentEntry = GetCurrentEntry();

	if (!currentEntry)
	{
		return false;
	}

	return !IsNamespaceRoot(currentEntry->GetPidl().get());
}

HRESULT ShellNavigationController::GoUp()
{
	auto *currentEntry = GetCurrentEntry();

	if (!currentEntry)
	{
		return E_FAIL;
	}

	unique_pidl_absolute pidlParent;
	HRESULT hr = GetVirtualParentPath(currentEntry->GetPidl().get(), wil::out_param(pidlParent));

	if (FAILED(hr))
	{
		return hr;
	}

	auto navigateParams = NavigateParams::Up(pidlParent.get(), currentEntry->GetPidl().get());

	if (m_navigationMode == NavigationMode::ForceNewTab && GetCurrentEntry())
	{
		m_tabNavigation->CreateNewTab(navigateParams, true);
		return S_OK;
	}
	else
	{
		return m_navigator->Navigate(navigateParams);
	}
}

HRESULT ShellNavigationController::Refresh()
{
	auto *currentEntry = GetCurrentEntry();

	if (!currentEntry)
	{
		return E_FAIL;
	}

	auto navigateParams = NavigateParams::History(currentEntry);
	return m_navigator->Navigate(navigateParams);
}

HRESULT ShellNavigationController::Navigate(const HistoryEntry *entry)
{
	auto navigateParams = NavigateParams::History(entry);
	return Navigate(navigateParams);
}

HRESULT ShellNavigationController::Navigate(const std::wstring &path)
{
	unique_pidl_absolute pidlDirectory;
	RETURN_IF_FAILED(
		SHParseDisplayName(path.c_str(), nullptr, wil::out_param(pidlDirectory), 0, nullptr));

	auto navigateParams = NavigateParams::Normal(pidlDirectory.get());
	return Navigate(navigateParams);
}

HRESULT ShellNavigationController::Navigate(NavigateParams &navigateParams)
{
	auto currentEntry = GetCurrentEntry();

	if (m_navigationMode == NavigationMode::ForceNewTab && currentEntry)
	{
		m_tabNavigation->CreateNewTab(navigateParams, true);
		return S_OK;
	}

	if (currentEntry
		&& ArePidlsEquivalent(currentEntry->GetPidl().get(), navigateParams.pidl.Raw()))
	{
		navigateParams.historyEntryType = HistoryEntryType::None;
	}

	return m_navigator->Navigate(navigateParams);
}

HRESULT ShellNavigationController::GetFailureValue()
{
	return E_FAIL;
}

void ShellNavigationController::SetNavigationMode(NavigationMode navigationMode)
{
	m_navigationMode = navigationMode;
}

HistoryEntry *ShellNavigationController::GetEntryById(int id)
{
	for (int i = 0; i < GetNumHistoryEntries(); i++)
	{
		auto *entry = GetEntryAtIndex(i);

		if (entry->GetId() == id)
		{
			return entry;
		}
	}

	return nullptr;
}
