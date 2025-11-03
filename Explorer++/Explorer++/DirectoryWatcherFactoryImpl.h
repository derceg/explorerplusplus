// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DirectoryWatcherFactory.h"
#include <concurrencpp/concurrencpp.h>

struct Config;
class ShellWatcherManager;

class DirectoryWatcherFactoryImpl : public DirectoryWatcherFactory
{
public:
	DirectoryWatcherFactoryImpl(const Config *config, ShellWatcherManager *shellWatcherManager,
		std::shared_ptr<concurrencpp::executor> uiThreadExecutor);

	std::unique_ptr<DirectoryWatcher> MaybeCreate(const PidlAbsolute &pidl,
		DirectoryWatcher::Filters filters, DirectoryWatcher::Callback callback,
		DirectoryWatcher::Behavior behavior) override;

private:
	const Config *const m_config;
	ShellWatcherManager *const m_shellWatcherManager;
	const std::shared_ptr<concurrencpp::executor> m_uiThreadExecutor;
};
