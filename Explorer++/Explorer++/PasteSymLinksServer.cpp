// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PasteSymLinksServer.h"
#include <boost/interprocess/sync/scoped_lock.hpp>

ClipboardOperations::PastedItems PasteSymLinksServer::LaunchClientAndWaitForResponse(
	std::function<bool()> clientLauncher, std::chrono::milliseconds responseTimeout)
{
	try
	{
		// Note that the segment (which is of type managed_windows_shared_memory) doesn't require
		// explicit removal, since it will be destroyed once all handles are closed.
		Segment segment(boost::interprocess::create_only, SHARED_MEMORY_NAME, SHARED_MEMORY_SIZE);

		const CharAllocator allocator(segment.get_segment_manager());
		auto *sharedData = segment.construct<SharedData>(SHARED_DATA_NAME)(allocator);

		boost::interprocess::scoped_lock lock(sharedData->resultMutex);

		if (!clientLauncher())
		{
			return {};
		}

		auto waitResult = sharedData->resultCondition.wait_for(lock, responseTimeout);

		if (waitResult == boost::interprocess::cv_status::timeout)
		{
			return {};
		}

		std::stringstream stringstream;
		stringstream << sharedData->serializedResultData;
		cereal::BinaryInputArchive inputArchive(stringstream);

		ClipboardOperations::PastedItems pastedItems;
		inputArchive(pastedItems);

		return pastedItems;
	}
	catch (const boost::interprocess::interprocess_exception &e)
	{
		// An exception of this type could indicate that there's already a shared memory segment
		// with this name, for example.
		LOG(ERROR) << e.what();
		return {};
	}
}
