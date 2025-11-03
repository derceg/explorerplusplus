// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DirectoryWatcher.h"
#include "../Helper/PidlHelper.h"
#include <memory>

class DirectoryWatcherFactory
{
public:
	virtual ~DirectoryWatcherFactory() = default;

	virtual std::unique_ptr<DirectoryWatcher> MaybeCreate(const PidlAbsolute &pidl,
		DirectoryWatcher::Filters filters, DirectoryWatcher::Callback callback,
		DirectoryWatcher::Behavior behavior) = 0;
};
