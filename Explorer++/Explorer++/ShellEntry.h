// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DirectoryWatcher.h"
#include "ShellItemFilter.h"
#include "../Helper/PassKey.h"
#include "../Helper/Pidl.h"
#include "../Helper/SignalWrapper.h"
#include <boost/container_hash/hash.hpp>
#include <boost/core/noncopyable.hpp>
#include <concurrencpp/concurrencpp.h>
#include <memory>
#include <unordered_map>
#include <vector>

class ShellContext;

// Represents an individual shell item (either a file or a folder). A folder can itself contain
// ShellEntry children, forming a hierarchical tree.
class ShellEntry : private boost::noncopyable
{
private:
	using PassKey = PassKey<ShellEntry>;

public:
	ShellEntry(const PidlAbsolute &pidl, ShellContext *shellContext,
		ShellItemFilter::ItemType childItemType,
		ShellItemFilter::HiddenItemPolicy hiddenItemPolicy);
	ShellEntry(const PidlAbsolute &pidl, ShellEntry *parent, ShellContext *shellContext,
		ShellItemFilter::ItemType childItemType, ShellItemFilter::HiddenItemPolicy hiddenItemPolicy,
		PassKey);

	const PidlAbsolute &GetPidl() const;
	const ShellEntry *GetParent() const;

	void LoadChildren();
	void UnloadChildren();
	bool AreChildrenLoaded() const;
	concurrencpp::generator<const ShellEntry *> GetChildren() const;
	ShellEntry *MaybeGetChild(const PidlAbsolute &pidl);

	// Signals
	SignalWrapper<ShellEntry, void(ShellEntry *childEntry)> childAddedSignal;
	SignalWrapper<ShellEntry, void(ShellEntry *childEntry)> childRemovedSignal;
	SignalWrapper<ShellEntry, void()> renamedSignal;
	SignalWrapper<ShellEntry, void()> updatedSignal;

	// ShellEntry can form a tree, with a top-level entry and entries expanded below it. When the
	// file/folder associated with a child ShellEntry is deleted, the entry can be removed directly
	// from its parent. However, when the file/folder associated with the top-level ShellEntry
	// is deleted, there is no ShellEntry parent; instead, the instance is owned by the client.
	//
	// This signal indicates that the file/folder associated with the top-level entry has been
	// deleted. In response, the client can destroy the top-level ShellEntry instance.
	SignalWrapper<ShellEntry, void()> topLevelEntryDeletedSignal;

private:
	std::vector<PidlChild> EnumerateChildren();

	void ProcessDirectoryChangeNotification(DirectoryWatcher::Event event,
		const PidlAbsolute &simplePidl1, const PidlAbsolute &simplePidl2);
	void OnChildAdded(const PidlAbsolute &simplePidl);
	void OnChildRenamed(const PidlAbsolute &simplePidlOld, const PidlAbsolute &simplePidlNew);
	void OnCurrentEntryRenamed(const PidlAbsolute &simplePidlNew);
	void ReplacePidlForRename(const PidlAbsolute &pidlNew);
	void OnChildPidlReplacedForRename(const PidlAbsolute &pidlOld, const PidlAbsolute &pidlNew);
	void RebuildPidlRecursive();
	void OnChildUpdated(const PidlAbsolute &simplePidl);
	void OnCurrentEntryUpdated();
	void OnChildRemoved(const PidlAbsolute &simplePidl);
	void OnCurrentEntryRemoved();

	void AddChild(const PidlAbsolute &pidl);

	void UpdateDirectoryWatcher();
	bool ShouldIncludeItem(const PidlAbsolute &pidl) const;
	bool HasAttributes(SFGAOF attributes) const;

	PidlAbsolute m_pidl;
	ShellEntry *const m_parent = nullptr;
	ShellContext *const m_shellContext;
	const ShellItemFilter::ItemType m_childItemType;
	const ShellItemFilter::HiddenItemPolicy m_hiddenItemPolicy;
	std::unordered_map<PidlAbsolute, std::unique_ptr<ShellEntry>, boost::hash<PidlAbsolute>>
		m_pidlToChildMap;
	bool m_childrenLoaded = false;
	std::unique_ptr<DirectoryWatcher> m_directoryWatcher;
};
