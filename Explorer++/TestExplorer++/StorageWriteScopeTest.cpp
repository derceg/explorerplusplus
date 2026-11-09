// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/StorageWriteScope.h"
#include <chrono>
#include <future>
#include <thread>

TEST(StorageWriteScopeTest, NestedScopeRestoresParentErrorAndTransaction)
{
	HRESULT outerError = S_OK;
	HRESULT innerError = S_OK;
	const auto outerTransaction = reinterpret_cast<HANDLE>(1);
	const auto innerTransaction = reinterpret_cast<HANDLE>(2);
	StorageWriteScope outer(outerError, outerTransaction);
	{
		StorageWriteScope inner(innerError, innerTransaction);
		EXPECT_EQ(StorageWriteScope::Transaction(), innerTransaction);
		StorageWriteScope::Record(HRESULT_FROM_WIN32(ERROR_CRC));
		EXPECT_EQ(outerError, S_OK);
	}
	EXPECT_EQ(StorageWriteScope::Transaction(), outerTransaction);
	StorageWriteScope::Record(E_ACCESSDENIED);
	StorageWriteScope::Record(E_FAIL);
	EXPECT_EQ(outerError, E_ACCESSDENIED);
	EXPECT_EQ(innerError, HRESULT_FROM_WIN32(ERROR_CRC));
}

TEST(StorageWriteScopeTest, ConcurrentScopesKeepErrorsOnTheirOwnThread)
{
	HRESULT mainError = S_OK;
	HRESULT workerError = S_OK;
	StorageWriteScope mainScope(mainError);
	std::promise<void> ready;
	std::promise<void> release;
	auto readyFuture = ready.get_future();
	auto releaseFuture = release.get_future();
	std::jthread worker(
		[&]
		{
			StorageWriteScope workerScope(workerError);
			ready.set_value();
			releaseFuture.wait();
			StorageWriteScope::Record(E_ABORT);
		});
	EXPECT_EQ(readyFuture.wait_for(std::chrono::seconds(5)), std::future_status::ready);
	StorageWriteScope::Record(E_ACCESSDENIED);
	release.set_value();
	worker.join();
	EXPECT_EQ(mainError, E_ACCESSDENIED);
	EXPECT_EQ(workerError, E_ABORT);
}
