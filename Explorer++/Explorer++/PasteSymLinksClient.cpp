// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PasteSymLinksClient.h"
#include <cereal/archives/binary.hpp>

void PasteSymLinksClient::NotifyServerOfResult(const ClipboardOperations::PastedItems &pastedItems)
{
	try
	{
		Segment segment(boost::interprocess::open_only, SHARED_MEMORY_NAME);

		auto [sharedData, length] = segment.find<SharedData>(SHARED_DATA_NAME);

		if (!sharedData || length != 1)
		{
			DCHECK(false);
			return;
		}

		std::stringstream stringstream;
		cereal::BinaryOutputArchive outputArchive(stringstream);

		outputArchive(pastedItems);

		sharedData->serializedResultData = stringstream.view();

		boost::interprocess::scoped_lock lock(sharedData->resultMutex);
		sharedData->resultCondition.notify_all();
	}
	catch (const boost::interprocess::interprocess_exception &e)
	{
		// If this exception is thrown, it indicates that the shared memory isn't present (i.e. the
		// server is gone).
		LOG(ERROR) << e.what();
	}
	catch (const boost::container::length_error &e)
	{
		// If this exception is thrown, it indicates that the shared memory isn't large enough to
		// hold the serialized string.
		LOG(ERROR) << e.what();
	}
}
