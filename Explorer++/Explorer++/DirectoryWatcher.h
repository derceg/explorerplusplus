// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/PidlHelper.h"
#include <functional>

class DirectoryWatcher
{
public:
	// This can be used to indicate the source events that a client is interested in. Note that
	// specifying an individual filter type doesn't necessarily mean that the client will be
	// notified only when that specific type of event occurs; other types of events can also trigger
	// notifications. For example, if FileAdded is specified, it doesn't necessarily mean that the
	// client will only be notified when a file is added.
	enum class Filters
	{
		FileAdded = 1 << 0,
		FileRenamed = 1 << 1,
		FileRemoved = 1 << 2,
		DirectoryAdded = 1 << 3,
		DirectoryRenamed = 1 << 4,
		DirectoryRemoved = 1 << 5,
		Modified = 1 << 6,
		Attributes = 1 << 7,
		All = FileAdded | FileRenamed | FileRemoved | DirectoryAdded | DirectoryRenamed
			| DirectoryRemoved | Modified | Attributes
	};

	enum class Behavior
	{
		NonRecursive,
		Recursive
	};

	enum class Event
	{
		Added,
		Renamed,
		Modified,
		DirectoryContentsChanged,
		Removed
	};

	using Callback = std::function<void(Event event, const PidlAbsolute &simplePidl1,
		const PidlAbsolute &simplePidl2)>;

	virtual ~DirectoryWatcher() = default;
};

DEFINE_ENUM_FLAG_OPERATORS(DirectoryWatcher::Filters);
