// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/managed_windows_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>

class PasteSymLinksServerClientBase
{
public:
	static constexpr size_t SHARED_MEMORY_SIZE = 64 * 1024;

	virtual ~PasteSymLinksServerClientBase() = default;

protected:
	using Segment = boost::interprocess::managed_windows_shared_memory;
	using Manager = Segment::segment_manager;
	template <typename T>
	using Allocator = boost::interprocess::allocator<T, Manager>;
	using CharAllocator = Allocator<char>;
	using String = boost::interprocess::basic_string<char, std::char_traits<char>, CharAllocator>;

	struct SharedData
	{
		boost::interprocess::interprocess_mutex resultMutex;
		boost::interprocess::interprocess_condition resultCondition;
		String serializedResultData;

		SharedData(const CharAllocator &allocator) : serializedResultData(allocator)
		{
		}
	};

	static constexpr char SHARED_MEMORY_NAME[] = "Explorer++PasteSymLinksSharedMemory";
	static constexpr char SHARED_DATA_NAME[] = "SharedData";
};
