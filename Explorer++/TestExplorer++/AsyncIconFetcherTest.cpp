// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "AsyncIconFetcher.h"
#include "MessageLoop.h"
#include "Runtime.h"
#include "RuntimeHelper.h"
#include "RuntimeTestHelper.h"
#include "ShellTestHelper.h"
#include "../Helper/CachedIcons.h"
#include <gtest/gtest.h>

using namespace testing;

class AsyncIconFetcherTest : public Test
{
protected:
	AsyncIconFetcherTest() :
		m_runtime(BuildRuntimeForTest()),
		m_cachedIcons(std::make_shared<CachedIcons>(10)),
		m_iconFetcher(&m_runtime, m_cachedIcons)
	{
	}

	template <typename T>
	T PumpMessagesUntilCoroutineFinished(const Runtime *runtime,
		concurrencpp::lazy_result<T> awaitableResult)
	{
		MessageLoop messageLoop;
		auto result = WaitForCoroutineResult(runtime, std::move(awaitableResult),
			[&messageLoop] { messageLoop.Stop(); });

		messageLoop.Run();

		return result.get();
	}

	template <typename T>
	concurrencpp::result<T> WaitForCoroutineResult(const Runtime *runtime,
		concurrencpp::lazy_result<T> awaitableResult, std::function<void()> finishedCallback)
	{
		auto result = co_await awaitableResult;
		co_await ResumeOnUiThread(runtime);
		finishedCallback();
		co_return result;
	}

	Runtime m_runtime;
	std::shared_ptr<CachedIcons> m_cachedIcons;
	AsyncIconFetcher m_iconFetcher;
};

TEST_F(AsyncIconFetcherTest, GetIconIndexAsync)
{
	std::wstring path = L"C:\\Fake";
	auto pidl = CreateSimplePidlForTest(path);
	std::stop_source stopSource;

	auto awaitableResult = m_iconFetcher.GetIconIndexAsync(pidl.Raw(), stopSource.get_token());
	auto iconIndex = PumpMessagesUntilCoroutineFinished(&m_runtime, std::move(awaitableResult));
	EXPECT_NE(iconIndex, std::nullopt);

	// Once an icon has been found, the cache should be updated.
	auto cachedIconIndex = m_cachedIcons->MaybeGetIconIndex(path);
	EXPECT_EQ(cachedIconIndex, iconIndex);
}

TEST_F(AsyncIconFetcherTest, GetIconIndexAsyncStop)
{
	std::wstring path = L"C:\\Fake";
	auto pidl = CreateSimplePidlForTest(path);
	std::stop_source stopSource;

	auto awaitableResult = m_iconFetcher.GetIconIndexAsync(pidl.Raw(), stopSource.get_token());

	stopSource.request_stop();
	auto iconIndex = PumpMessagesUntilCoroutineFinished(&m_runtime, std::move(awaitableResult));
	EXPECT_EQ(iconIndex, std::nullopt);

	// Since the request was stopped before it completed, the cache shouldn't have been updated.
	auto cachedIconIndex = m_cachedIcons->MaybeGetIconIndex(path);
	EXPECT_EQ(cachedIconIndex, std::nullopt);
}
