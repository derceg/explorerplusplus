// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellEntryCollection.h"
#include "ShellEntry.h"
#include "../Helper/Pidl.h"
#include <algorithm>

ShellEntryCollection::ShellEntryCollection(ShellContext *shellContext,
	ShellItemFilter::ItemType childItemType, ShellItemFilter::HiddenItemPolicy hiddenItemPolicy) :
	m_shellContext(shellContext),
	m_childItemType(childItemType),
	m_hiddenItemPolicy(hiddenItemPolicy)
{
}

ShellEntry *ShellEntryCollection::AddTopLevelEntry(const PidlAbsolute &pidl)
{
	auto shellEntry =
		std::make_unique<ShellEntry>(pidl, m_shellContext, m_childItemType, m_hiddenItemPolicy);
	auto *rawShellEntry = shellEntry.get();
	m_topLevelEntries.push_back(std::move(shellEntry));

	OnShellEntryAdded(rawShellEntry);

	return rawShellEntry;
}

concurrencpp::generator<ShellEntry *> ShellEntryCollection::GetTopLevelEntries()
{
	for (const auto &topLevelEntry : m_topLevelEntries)
	{
		co_yield topLevelEntry.get();
	}
}

size_t ShellEntryCollection::GetTopLevelEntryIndex(const ShellEntry *shellEntry) const
{
	auto itr = std::ranges::find(m_topLevelEntries, shellEntry,
		[](const auto &currentShellEntry) { return currentShellEntry.get(); });
	CHECK(itr != m_topLevelEntries.end());
	return std::distance(m_topLevelEntries.begin(), itr);
}

ShellEntry *ShellEntryCollection::MaybeLoadEntryForPidl(const PidlAbsolute &pidl)
{
	for (const auto &child : m_topLevelEntries)
	{
		auto *entry = MaybeLoadEntryForPidl(pidl, child.get());

		if (entry)
		{
			return entry;
		}
	}

	return nullptr;
}

ShellEntry *ShellEntryCollection::MaybeLoadEntryForPidl(const PidlAbsolute &pidl,
	ShellEntry *currentEntry)
{
	if (!currentEntry->GetPidl().IsAncestor(pidl))
	{
		return nullptr;
	}

	if (currentEntry->GetPidl() == pidl)
	{
		return currentEntry;
	}

	currentEntry->LoadChildren();

	for (auto *child : currentEntry->GetChildren())
	{
		if (auto *shellEntry = MaybeLoadEntryForPidl(pidl, child))
		{
			return shellEntry;
		}
	}

	return nullptr;
}

void ShellEntryCollection::OnShellEntryAdded(ShellEntry *shellEntry)
{
	AddEntryObservers(shellEntry);

	entryAddedSignal.m_signal(shellEntry);
}

void ShellEntryCollection::OnShellEntryRenamed(ShellEntry *shellEntry)
{
	entryRenamedSignal.m_signal(shellEntry);
}

void ShellEntryCollection::OnShellEntryUpdated(ShellEntry *shellEntry)
{
	entryUpdatedSignal.m_signal(shellEntry);
}

void ShellEntryCollection::OnShellEntryRemoved(ShellEntry *shellEntry)
{
	entryRemovedSignal.m_signal(shellEntry);
}

void ShellEntryCollection::OnTopLevelEntryDeleted(ShellEntry *shellEntry)
{
	auto itr = std::ranges::find(m_topLevelEntries, shellEntry,
		[](const auto &currentShellEntry) { return currentShellEntry.get(); });
	CHECK(itr != m_topLevelEntries.end());

	auto ownedEntry = std::move(*itr);
	m_topLevelEntries.erase(itr);

	OnShellEntryRemoved(shellEntry);
}

void ShellEntryCollection::AddEntryObservers(ShellEntry *shellEntry)
{
	// These observers don't need to be removed, since this class owns all entries.
	shellEntry->childAddedSignal.AddObserver(
		std::bind_front(&ShellEntryCollection::OnShellEntryAdded, this));
	shellEntry->renamedSignal.AddObserver(
		std::bind_front(&ShellEntryCollection::OnShellEntryRenamed, this, shellEntry));
	shellEntry->updatedSignal.AddObserver(
		std::bind_front(&ShellEntryCollection::OnShellEntryUpdated, this, shellEntry));
	shellEntry->childRemovedSignal.AddObserver(
		std::bind_front(&ShellEntryCollection::OnShellEntryRemoved, this));
	shellEntry->topLevelEntryDeletedSignal.AddObserver(
		std::bind_front(&ShellEntryCollection::OnTopLevelEntryDeleted, this, shellEntry));
}
