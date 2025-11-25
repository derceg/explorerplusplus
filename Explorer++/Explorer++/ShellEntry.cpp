// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellEntry.h"
#include "DirectoryWatcherFactory.h"
#include "PidlUpdater.h"
#include "ShellContext.h"
#include "ShellEnumerator.h"
#include "../Helper/ShellHelper.h"
#include <ranges>
#include <utility>

ShellEntry::ShellEntry(const PidlAbsolute &pidl, ShellContext *shellContext,
	ShellItemFilter::ItemType childItemType, ShellItemFilter::HiddenItemPolicy hiddenItemPolicy) :
	m_pidl(pidl),
	m_shellContext(shellContext),
	m_childItemType(childItemType),
	m_hiddenItemPolicy(hiddenItemPolicy)
{
}

ShellEntry::ShellEntry(const PidlAbsolute &pidl, ShellEntry *parent, ShellContext *shellContext,
	ShellItemFilter::ItemType childItemType, ShellItemFilter::HiddenItemPolicy hiddenItemPolicy,
	PassKey) :
	m_pidl(pidl),
	m_parent(parent),
	m_shellContext(shellContext),
	m_childItemType(childItemType),
	m_hiddenItemPolicy(hiddenItemPolicy)
{
}

const PidlAbsolute &ShellEntry::GetPidl() const
{
	return m_pidl;
}

const ShellEntry *ShellEntry::GetParent() const
{
	return m_parent;
}

void ShellEntry::LoadChildren()
{
	if (!HasAttributes(SFGAO_FOLDER))
	{
		DCHECK(false);
		return;
	}

	if (m_childrenLoaded)
	{
		return;
	}

	auto childPidls = EnumerateChildren();

	for (const auto &childPidl : childPidls)
	{
		AddChild(m_pidl + childPidl);
	}

	m_childrenLoaded = true;

	UpdateDirectoryWatcher();
}

std::vector<PidlChild> ShellEntry::EnumerateChildren()
{
	std::vector<PidlChild> childPidls;
	HRESULT hr = m_shellContext->GetShellEnumerator()->EnumerateDirectory(m_pidl.Raw(),
		m_childItemType, m_hiddenItemPolicy, childPidls, std::stop_token());

	if (FAILED(hr))
	{
		return {};
	}

	return childPidls;
}

void ShellEntry::UnloadChildren()
{
	if (!HasAttributes(SFGAO_FOLDER))
	{
		DCHECK(false);
		return;
	}

	if (!m_childrenLoaded)
	{
		return;
	}

	for (auto itr = m_pidlToChildMap.begin(); itr != m_pidlToChildMap.end();)
	{
		auto ownedEntry = std::move(itr->second);
		itr = m_pidlToChildMap.erase(itr);

		childRemovedSignal.m_signal(ownedEntry.get());
	}

	m_childrenLoaded = false;

	UpdateDirectoryWatcher();
}

bool ShellEntry::AreChildrenLoaded() const
{
	DCHECK(HasAttributes(SFGAO_FOLDER));

	return m_childrenLoaded;
}

concurrencpp::generator<const ShellEntry *> ShellEntry::GetChildren() const
{
	DCHECK(HasAttributes(SFGAO_FOLDER));

	for (const auto &child : m_pidlToChildMap | std::views::values)
	{
		co_yield child.get();
	}
}

ShellEntry *ShellEntry::MaybeGetChild(const PidlAbsolute &pidl)
{
	auto itr = m_pidlToChildMap.find(pidl);

	if (itr == m_pidlToChildMap.end())
	{
		return nullptr;
	}

	return itr->second.get();
}

void ShellEntry::ProcessDirectoryChangeNotification(DirectoryWatcher::Event event,
	const PidlAbsolute &simplePidl1, const PidlAbsolute &simplePidl2)
{
	switch (event)
	{
	case DirectoryWatcher::Event::Added:
		if (m_pidl.IsParent(simplePidl1))
		{
			OnChildAdded(simplePidl1);
		}
		break;

	case DirectoryWatcher::Event::Renamed:
		if (m_pidl.IsParent(simplePidl1))
		{
			OnChildRenamed(simplePidl1, simplePidl2);
		}
		else if (m_pidl == simplePidl1)
		{
			OnCurrentEntryRenamed(simplePidl2);
		}
		break;

	case DirectoryWatcher::Event::Modified:
		if (m_pidl.IsParent(simplePidl1))
		{
			OnChildUpdated(simplePidl1);
		}
		else if (m_pidl == simplePidl1)
		{
			OnCurrentEntryUpdated();
		}
		break;

	case DirectoryWatcher::Event::DirectoryContentsChanged:
		// At the moment, this case isn't handled.
		break;

	case DirectoryWatcher::Event::Removed:
		if (m_pidl.IsParent(simplePidl1))
		{
			OnChildRemoved(simplePidl1);
		}
		else if (m_pidl == simplePidl1)
		{
			OnCurrentEntryRemoved();
		}
		break;
	}
}

void ShellEntry::OnChildAdded(const PidlAbsolute &simplePidl)
{
	if (MaybeGetChild(simplePidl))
	{
		return;
	}

	auto updatedPidl = m_shellContext->GetPidlUpdater()->GetUpdatedPidl(simplePidl);
	AddChild(updatedPidl);
}

void ShellEntry::OnChildRenamed(const PidlAbsolute &simplePidlOld,
	const PidlAbsolute &simplePidlNew)
{
	auto *child = MaybeGetChild(simplePidlOld);

	if (!child)
	{
		// One way this path can be taken is if ChildType is set to FoldersOnly and a container file
		// starts with a different extension, before being renamed. For example, if a .zip file is
		// created with the extension .zip.tmp, it wouldn't be added initially (since .zip.tmp isn't
		// a container file type). When the file is then renamed to .zip, it should be added.
		OnChildAdded(simplePidlNew);
		return;
	}

	child->OnCurrentEntryRenamed(simplePidlNew);
}

void ShellEntry::OnCurrentEntryRenamed(const PidlAbsolute &simplePidlNew)
{
	auto updatedPidlNew = m_shellContext->GetPidlUpdater()->GetUpdatedPidl(simplePidlNew);

	if (!ShouldIncludeItem(updatedPidlNew))
	{
		// This path can be taken if a container file has its extension changed. For example, if a
		// .zip file has its extension changed to .zip.bak. In that case, the file needs to be
		// removed, as it's no longer a container file.
		OnCurrentEntryRemoved();
		return;
	}

	if (!DoesItemHaveAttributes(updatedPidlNew.Raw(), SFGAO_FOLDER) && m_childrenLoaded)
	{
		UnloadChildren();
	}

	ReplacePidlForRename(updatedPidlNew);
}

void ShellEntry::ReplacePidlForRename(const PidlAbsolute &pidlNew)
{
	auto pidlOld = m_pidl;
	m_pidl = pidlNew;

	if (m_parent)
	{
		m_parent->OnChildPidlReplacedForRename(pidlOld, m_pidl);
	}

	RebuildPidlRecursive();

	renamedSignal.m_signal();
}

void ShellEntry::OnChildPidlReplacedForRename(const PidlAbsolute &pidlOld,
	const PidlAbsolute &pidlNew)
{
	auto nodeHandler = m_pidlToChildMap.extract(pidlOld);
	CHECK(nodeHandler);

	nodeHandler.key() = pidlNew;
	auto result = m_pidlToChildMap.insert(std::move(nodeHandler));
	CHECK(result.inserted);
}

void ShellEntry::RebuildPidlRecursive()
{
	if (m_parent)
	{
		auto childPidl = m_pidl.GetLastItem();
		CHECK(childPidl.HasValue());

		m_pidl = m_parent->GetPidl() + childPidl;
	}

	if (!m_childrenLoaded)
	{
		return;
	}

	for (auto &child : m_pidlToChildMap | std::views::values)
	{
		child->RebuildPidlRecursive();
	}

	auto previousMap = std::exchange(m_pidlToChildMap, {});

	for (auto &entry : previousMap | std::views::values)
	{
		m_pidlToChildMap.insert({ entry->GetPidl(), std::move(entry) });
	}

	UpdateDirectoryWatcher();
}

void ShellEntry::OnChildUpdated(const PidlAbsolute &simplePidl)
{
	auto *child = MaybeGetChild(simplePidl);

	if (!child)
	{
		// If hidden items are excluded, this path can be taken when a hidden item is unhidden.
		OnChildAdded(simplePidl);
		return;
	}

	child->OnCurrentEntryUpdated();
}

void ShellEntry::OnCurrentEntryUpdated()
{
	m_pidl = m_shellContext->GetPidlUpdater()->GetUpdatedPidl(m_pidl);

	if (!ShouldIncludeItem(m_pidl))
	{
		// If hidden items are excluded, this path can be taken when an item transitions from
		// non-hidden to hidden.
		OnCurrentEntryRemoved();
		return;
	}

	updatedSignal.m_signal();
}

void ShellEntry::OnChildRemoved(const PidlAbsolute &simplePidl)
{
	auto itr = m_pidlToChildMap.find(simplePidl);

	if (itr == m_pidlToChildMap.end())
	{
		return;
	}

	auto ownedEntry = std::move(itr->second);
	m_pidlToChildMap.erase(itr);

	childRemovedSignal.m_signal(ownedEntry.get());
}

void ShellEntry::OnCurrentEntryRemoved()
{
	if (!m_parent)
	{
		topLevelEntryDeletedSignal.m_signal();
		return;
	}

	m_parent->OnChildRemoved(m_pidl);
}

void ShellEntry::AddChild(const PidlAbsolute &pidl)
{
	if (!m_pidl.IsParent(pidl))
	{
		DCHECK(false);
		return;
	}

	if (!ShouldIncludeItem(pidl))
	{
		return;
	}

	auto entry = std::make_unique<ShellEntry>(pidl, this, m_shellContext, m_childItemType,
		m_hiddenItemPolicy, PassKey());
	auto *rawEntry = entry.get();

	auto [itr, didInsert] = m_pidlToChildMap.insert({ pidl, std::move(entry) });
	CHECK(didInsert);

	childAddedSignal.m_signal(rawEntry);
}

void ShellEntry::UpdateDirectoryWatcher()
{
	if (m_childrenLoaded)
	{
		m_directoryWatcher = m_shellContext->GetDirectoryWatcherFactory()->MaybeCreate(m_pidl,
			DirectoryWatcher::Filters::All,
			std::bind_front(&ShellEntry::ProcessDirectoryChangeNotification, this),
			DirectoryWatcher::Behavior::NonRecursive);
	}
	else
	{
		m_directoryWatcher.reset();
	}
}

bool ShellEntry::ShouldIncludeItem(const PidlAbsolute &pidl) const
{
	if (m_childItemType == ShellItemFilter::ItemType::FoldersOnly
		&& !DoesItemHaveAttributes(pidl.Raw(), SFGAO_FOLDER))
	{
		return false;
	}

	if (m_hiddenItemPolicy == ShellItemFilter::HiddenItemPolicy::Exclude
		&& DoesItemHaveAttributes(pidl.Raw(), SFGAO_HIDDEN))
	{
		return false;
	}

	return true;
}

bool ShellEntry::HasAttributes(SFGAOF attributes) const
{
	return DoesItemHaveAttributes(m_pidl.Raw(), attributes);
}
