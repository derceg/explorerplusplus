// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellItemFilter.h"
#include "../Helper/SignalWrapper.h"
#include <concurrencpp/concurrencpp.h>
#include <memory>
#include <vector>

class PidlAbsolute;
class ShellContext;
class ShellEntry;

// Holds a set of unrelated ShellEntry instances. This models a set of shell items as they would be
// shown in a treeview. That is, there can be multiple, unrelated, top-level items, where each
// top-level item (represented by an individual ShellEntry) can be expanded in an arbitrary way.
class ShellEntryCollection
{
public:
	ShellEntryCollection(ShellContext *shellContext, ShellItemFilter::ItemType childItemType,
		ShellItemFilter::HiddenItemPolicy hiddenItemPolicy);

	ShellEntry *AddTopLevelEntry(const PidlAbsolute &pidl);
	concurrencpp::generator<ShellEntry *> GetTopLevelEntries();
	size_t GetTopLevelEntryIndex(const ShellEntry *shellEntry) const;

	ShellEntry *MaybeLoadEntryForPidl(const PidlAbsolute &pidl);

	// Signals
	SignalWrapper<ShellEntryCollection, void(ShellEntry *shellEntry)> entryAddedSignal;
	SignalWrapper<ShellEntryCollection, void(ShellEntry *shellEntry)> entryRenamedSignal;
	SignalWrapper<ShellEntryCollection, void(ShellEntry *shellEntry)> entryUpdatedSignal;
	SignalWrapper<ShellEntryCollection, void(ShellEntry *shellEntry)> entryRemovedSignal;

private:
	void OnShellEntryAdded(ShellEntry *shellEntry);
	void OnShellEntryRenamed(ShellEntry *shellEntry);
	void OnShellEntryUpdated(ShellEntry *shellEntry);
	void OnShellEntryRemoved(ShellEntry *shellEntry);
	void OnTopLevelEntryDeleted(ShellEntry *shellEntry);

	void AddEntryObservers(ShellEntry *shellEntry);

	ShellEntry *MaybeLoadEntryForPidl(const PidlAbsolute &pidl, ShellEntry *currentEntry);

	ShellContext *const m_shellContext;
	const ShellItemFilter::ItemType m_childItemType;
	const ShellItemFilter::HiddenItemPolicy m_hiddenItemPolicy;
	std::vector<std::unique_ptr<ShellEntry>> m_topLevelEntries;
};
