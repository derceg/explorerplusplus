// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DirectoryWatcherFactoryImpl.h"
#include "ChangeNotifyMode.h"
#include "Config.h"
#include "FileSystemWatcher.h"
#include "ShellWatcher.h"

DirectoryWatcherFactoryImpl::DirectoryWatcherFactoryImpl(const Config *config,
	ShellWatcherManager *shellWatcherManager,
	std::shared_ptr<concurrencpp::executor> uiThreadExecutor) :
	m_config(config),
	m_shellWatcherManager(shellWatcherManager),
	m_uiThreadExecutor(uiThreadExecutor)
{
}

std::unique_ptr<DirectoryWatcher> DirectoryWatcherFactoryImpl::MaybeCreate(const PidlAbsolute &pidl,
	DirectoryWatcher::Filters filters, DirectoryWatcher::Callback callback,
	DirectoryWatcher::Behavior behavior)
{
	if (m_config->changeNotifyMode == ChangeNotifyMode::Shell)
	{
		return ShellWatcher::MaybeCreate(m_shellWatcherManager, pidl, filters, callback, behavior);
	}
	else
	{
		return FileSystemWatcher::MaybeCreate(pidl, filters, m_uiThreadExecutor, callback,
			behavior);
	}
}
