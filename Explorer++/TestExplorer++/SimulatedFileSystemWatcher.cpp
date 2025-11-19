// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SimulatedFileSystemWatcher.h"
#include "SimulatedFileSystem.h"

SimulatedFileSystemWatcher::SimulatedFileSystemWatcher(SimulatedFileSystem *fileSystem,
	const PidlAbsolute &pidl, Callback callback, Behavior behavior) :
	m_pidl(pidl),
	m_callback(callback),
	m_behavior(behavior)
{
	m_connections.push_back(fileSystem->itemAddedSignal.AddObserver(
		std::bind_front(&SimulatedFileSystemWatcher::OnItemAdded, this)));
	m_connections.push_back(fileSystem->itemRenamedSignal.AddObserver(
		std::bind_front(&SimulatedFileSystemWatcher::OnItemRenamed, this)));
	m_connections.push_back(fileSystem->itemUpdatedSignal.AddObserver(
		std::bind_front(&SimulatedFileSystemWatcher::OnItemUpdated, this)));
	m_connections.push_back(fileSystem->directoryContentsChangedSignal.AddObserver(
		std::bind_front(&SimulatedFileSystemWatcher::OnDirectoryContentsChanged, this)));
	m_connections.push_back(fileSystem->itemRemovedSignal.AddObserver(
		std::bind_front(&SimulatedFileSystemWatcher::OnItemRemoved, this)));
}

void SimulatedFileSystemWatcher::OnItemAdded(const PidlAbsolute &pidl)
{
	MaybeDispatchNotification(Event::Added, pidl, {});
}

void SimulatedFileSystemWatcher::OnItemRenamed(const PidlAbsolute &pidlOld,
	const PidlAbsolute &pidlNew)
{
	MaybeDispatchNotification(Event::Renamed, pidlOld, pidlNew);
}

void SimulatedFileSystemWatcher::OnItemUpdated(const PidlAbsolute &pidl)
{
	MaybeDispatchNotification(Event::Modified, pidl, {});
}

void SimulatedFileSystemWatcher::OnDirectoryContentsChanged(const PidlAbsolute &pidl)
{
	MaybeDispatchNotification(Event::DirectoryContentsChanged, pidl, {});
}

void SimulatedFileSystemWatcher::OnItemRemoved(const PidlAbsolute &pidl)
{
	MaybeDispatchNotification(Event::Removed, pidl, {});
}

void SimulatedFileSystemWatcher::MaybeDispatchNotification(Event event, const PidlAbsolute &pidl1,
	const PidlAbsolute &pidl2)
{
	if (pidl1 == m_pidl || m_pidl.IsParent(pidl1)
		|| (m_behavior == Behavior::Recursive && m_pidl.IsAncestor(pidl1)))
	{
		m_callback(event, pidl1, pidl2);
	}
}
