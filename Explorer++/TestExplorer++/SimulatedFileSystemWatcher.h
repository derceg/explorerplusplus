// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DirectoryWatcher.h"
#include "../Helper/PidlHelper.h"
#include <boost/signals2.hpp>
#include <vector>

class SimulatedFileSystem;

class SimulatedFileSystemWatcher : public DirectoryWatcher
{
public:
	SimulatedFileSystemWatcher(SimulatedFileSystem *fileSystem, const PidlAbsolute &pidl,
		Callback callback, Behavior behavior);

private:
	void OnItemAdded(const PidlAbsolute &pidl);
	void OnItemRenamed(const PidlAbsolute &pidlOld, const PidlAbsolute &pidlNew);
	void OnItemUpdated(const PidlAbsolute &pidl);
	void OnDirectoryContentsChanged(const PidlAbsolute &pidl);
	void OnItemRemoved(const PidlAbsolute &pidl);
	void MaybeDispatchNotification(Event event, const PidlAbsolute &pidl1,
		const PidlAbsolute &pidl2);

	const PidlAbsolute m_pidl;
	const Callback m_callback;
	const Behavior m_behavior;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
