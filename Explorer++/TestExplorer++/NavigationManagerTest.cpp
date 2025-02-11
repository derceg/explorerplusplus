// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellBrowser/NavigationManager.h"
#include "ShellEnumeratorFake.h"
#include "ShellTestHelper.h"
#include <gtest/gtest.h>
#include <future>

using namespace testing;

class NavigationManagerTest : public Test
{
protected:
	NavigationManagerTest() :
		m_shellEnumerator(std::make_shared<ShellEnumeratorFake>()),
		m_manualExecutorBackground(std::make_shared<concurrencpp::manual_executor>()),
		m_manualExecutorCurrent(std::make_shared<concurrencpp::manual_executor>()),
		m_navigationManager(std::make_unique<NavigationManager>(m_shellEnumerator,
			m_manualExecutorBackground, m_manualExecutorCurrent))
	{
	}

	~NavigationManagerTest()
	{
		m_manualExecutorBackground->shutdown();
		m_manualExecutorCurrent->shutdown();
	}

	// Starts and then completes a navigation before returning.
	void CompleteNavigation(const NavigateParams &navigateParams)
	{
		m_navigationManager->StartNavigation(navigateParams);
		m_manualExecutorBackground->loop(std::numeric_limits<size_t>::max());
		m_manualExecutorCurrent->loop(std::numeric_limits<size_t>::max());
	}

	const std::shared_ptr<ShellEnumeratorFake> m_shellEnumerator;
	const std::shared_ptr<concurrencpp::manual_executor> m_manualExecutorBackground;
	const std::shared_ptr<concurrencpp::manual_executor> m_manualExecutorCurrent;
	std::unique_ptr<NavigationManager> m_navigationManager;
};

// The mocks here are all strict, which means that this should only be used as a base class when
// testing that the appropriate mock methods are called.
class NavigationManagerSignalTest : public NavigationManagerTest
{
protected:
	NavigationManagerSignalTest()
	{
		m_navigationManager->AddNavigationStartedObserver(
			m_navigationStartedCallback.AsStdFunction());

		m_navigationManager->AddNavigationWillCommitObserver(
			m_navigationWillCommitCallback.AsStdFunction());
		m_navigationManager->AddNavigationCommittedObserver(
			m_navigationCommittedCallback.AsStdFunction());
		m_navigationManager->AddNavigationItemsAvailableObserver(
			m_navigationItemsAvailableCallback.AsStdFunction());
		m_navigationManager->AddNavigationCompletedObserver(
			m_navigationCompletedCallback.AsStdFunction());

		m_navigationManager->AddNavigationFailedObserver(
			m_navigationFailedCallback.AsStdFunction());

		m_navigationManager->AddNavigationCancelledObserver(
			m_navigationCancelledCallback.AsStdFunction());
	}

	StrictMock<MockFunction<void(const NavigateParams &navigateParams)>>
		m_navigationStartedCallback;

	StrictMock<MockFunction<void(const NavigateParams &navigateParams)>>
		m_navigationWillCommitCallback;
	StrictMock<MockFunction<void(const NavigateParams &navigateParams)>>
		m_navigationCommittedCallback;
	StrictMock<MockFunction<void(const NavigateParams &navigateParams,
		const std::vector<PidlChild> &items)>>
		m_navigationItemsAvailableCallback;
	StrictMock<MockFunction<void(const NavigateParams &navigateParams)>>
		m_navigationCompletedCallback;

	StrictMock<MockFunction<void(const NavigateParams &navigateParams)>> m_navigationFailedCallback;

	StrictMock<MockFunction<void(const NavigateParams &navigateParams)>>
		m_navigationCancelledCallback;
};

TEST_F(NavigationManagerSignalTest, SuccessfulNavigation)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams = NavigateParams::Normal(pidl.Raw());

	{
		InSequence seq;

		EXPECT_CALL(m_navigationStartedCallback, Call(navigateParams));
		EXPECT_CALL(m_navigationWillCommitCallback, Call(navigateParams));
		EXPECT_CALL(m_navigationCommittedCallback, Call(navigateParams));
		EXPECT_CALL(m_navigationItemsAvailableCallback, Call(navigateParams, IsEmpty()));
		EXPECT_CALL(m_navigationCompletedCallback, Call(navigateParams));
	}

	CompleteNavigation(navigateParams);
}

TEST_F(NavigationManagerSignalTest, FailedInitialNavigation)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams = NavigateParams::Normal(pidl.Raw());

	{
		InSequence seq;

		// Even though the navigation failed, it should still be committed, since it was the first
		// navigation.
		EXPECT_CALL(m_navigationStartedCallback, Call(navigateParams));
		EXPECT_CALL(m_navigationWillCommitCallback, Call(navigateParams));
		EXPECT_CALL(m_navigationCommittedCallback, Call(navigateParams));
		EXPECT_CALL(m_navigationItemsAvailableCallback, Call(navigateParams, IsEmpty()));
		EXPECT_CALL(m_navigationCompletedCallback, Call(navigateParams));
	}

	m_shellEnumerator->SetShouldSucceed(false);
	CompleteNavigation(navigateParams);
}

TEST_F(NavigationManagerSignalTest, FailedSubsequentNavigation)
{
	PidlAbsolute pidlSuccess = CreateSimplePidlForTest(L"c:\\");
	auto navigateParamsSuccess = NavigateParams::Normal(pidlSuccess.Raw());

	PidlAbsolute pidlFail = CreateSimplePidlForTest(L"d:\\");
	auto navigateParamsFail = NavigateParams::Normal(pidlFail.Raw());

	{
		InSequence seq;

		EXPECT_CALL(m_navigationStartedCallback, Call(navigateParamsSuccess));
		EXPECT_CALL(m_navigationWillCommitCallback, Call(navigateParamsSuccess));
		EXPECT_CALL(m_navigationCommittedCallback, Call(navigateParamsSuccess));
		EXPECT_CALL(m_navigationItemsAvailableCallback, Call(navigateParamsSuccess, IsEmpty()));
		EXPECT_CALL(m_navigationCompletedCallback, Call(navigateParamsSuccess));
		EXPECT_CALL(m_navigationStartedCallback, Call(navigateParamsFail));
		EXPECT_CALL(m_navigationFailedCallback, Call(navigateParamsFail));
	}

	CompleteNavigation(navigateParamsSuccess);

	m_shellEnumerator->SetShouldSucceed(false);
	CompleteNavigation(navigateParamsFail);
}

TEST_F(NavigationManagerSignalTest, CommitWithPendingNavigation)
{
	PidlAbsolute pidl1 = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams1 = NavigateParams::Normal(pidl1.Raw());

	PidlAbsolute pidl2 = CreateSimplePidlForTest(L"d:\\");
	auto navigateParams2 = NavigateParams::Normal(pidl2.Raw());

	{
		InSequence seq;

		EXPECT_CALL(m_navigationStartedCallback, Call(navigateParams1));
		EXPECT_CALL(m_navigationStartedCallback, Call(navigateParams2));
		EXPECT_CALL(m_navigationWillCommitCallback, Call(navigateParams1));
		EXPECT_CALL(m_navigationCommittedCallback, Call(navigateParams1));
		EXPECT_CALL(m_navigationItemsAvailableCallback, Call(navigateParams1, IsEmpty()));
		EXPECT_CALL(m_navigationCompletedCallback, Call(navigateParams1));
		EXPECT_CALL(m_navigationCancelledCallback, Call(navigateParams2));
	}

	m_navigationManager->StartNavigation(navigateParams1);
	m_navigationManager->StartNavigation(navigateParams2);

	// This should allow the first navigation to proceed, allowing it to commit. That will then
	// cause the second navigation to be cancelled.
	m_manualExecutorBackground->loop(std::numeric_limits<size_t>::max());
	m_manualExecutorCurrent->loop(std::numeric_limits<size_t>::max());
}

TEST_F(NavigationManagerSignalTest, ManagerDestroyedWithPendingNavigation)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams = NavigateParams::Normal(pidl.Raw());

	{
		InSequence seq;

		// Navigations are always synchronously started, so this signal should always be emitted.
		EXPECT_CALL(m_navigationStartedCallback, Call(navigateParams));
	}

	m_navigationManager->StartNavigation(navigateParams);

	m_navigationManager.reset();

	// This will resume the asynchronous navigation. However, because the NavigationManager instance
	// was destroyed, no signals should be emitted. This should be a safe operation.
	m_manualExecutorBackground->loop(std::numeric_limits<size_t>::max());
	m_manualExecutorCurrent->loop(std::numeric_limits<size_t>::max());
}

class NavigationManagerSignalThreadTest : public NavigationManagerSignalTest
{
protected:
	NavigationManagerSignalThreadTest()
	{
		auto originalThreadId = UniqueThreadId::GetForCurrentThread();

		// The callbacks should all be invoked on the original thread, even if the enumeration is
		// done on a background thread.
		ON_CALL(m_navigationStartedCallback, Call)
			.WillByDefault([originalThreadId](const NavigateParams &)
				{ EXPECT_EQ(UniqueThreadId::GetForCurrentThread(), originalThreadId); });

		ON_CALL(m_navigationWillCommitCallback, Call)
			.WillByDefault([originalThreadId](const NavigateParams &)
				{ EXPECT_EQ(UniqueThreadId::GetForCurrentThread(), originalThreadId); });
		ON_CALL(m_navigationCommittedCallback, Call)
			.WillByDefault([originalThreadId](const NavigateParams &)
				{ EXPECT_EQ(UniqueThreadId::GetForCurrentThread(), originalThreadId); });
		ON_CALL(m_navigationItemsAvailableCallback, Call)
			.WillByDefault(
				[originalThreadId](const NavigateParams &, const std::vector<PidlChild> &)
				{ EXPECT_EQ(UniqueThreadId::GetForCurrentThread(), originalThreadId); });
		ON_CALL(m_navigationCompletedCallback, Call)
			.WillByDefault([originalThreadId](const NavigateParams &)
				{ EXPECT_EQ(UniqueThreadId::GetForCurrentThread(), originalThreadId); });

		ON_CALL(m_navigationFailedCallback, Call)
			.WillByDefault([originalThreadId](const NavigateParams &)
				{ EXPECT_EQ(UniqueThreadId::GetForCurrentThread(), originalThreadId); });

		ON_CALL(m_navigationCancelledCallback, Call)
			.WillByDefault([originalThreadId](const NavigateParams &)
				{ EXPECT_EQ(UniqueThreadId::GetForCurrentThread(), originalThreadId); });
	}
};

TEST_F(NavigationManagerSignalThreadTest, CheckThread)
{
	PidlAbsolute pidl1 = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams1 = NavigateParams::Normal(pidl1.Raw());

	PidlAbsolute pidl2 = CreateSimplePidlForTest(L"d:\\");
	auto navigateParams2 = NavigateParams::Normal(pidl2.Raw());

	PidlAbsolute pidl3 = CreateSimplePidlForTest(L"e:\\");
	auto navigateParams3 = NavigateParams::Normal(pidl3.Raw());

	{
		InSequence seq;

		EXPECT_CALL(m_navigationStartedCallback, Call(navigateParams1));
		EXPECT_CALL(m_navigationStartedCallback, Call(navigateParams2));
		EXPECT_CALL(m_navigationWillCommitCallback, Call(navigateParams1));
		EXPECT_CALL(m_navigationCommittedCallback, Call(navigateParams1));
		EXPECT_CALL(m_navigationItemsAvailableCallback, Call(navigateParams1, IsEmpty()));
		EXPECT_CALL(m_navigationCompletedCallback, Call(navigateParams1));
		EXPECT_CALL(m_navigationCancelledCallback, Call(navigateParams2));

		EXPECT_CALL(m_navigationStartedCallback, Call(navigateParams3));
		EXPECT_CALL(m_navigationFailedCallback, Call(navigateParams3));
	}

	m_navigationManager->StartNavigation(navigateParams1);
	m_navigationManager->StartNavigation(navigateParams2);

	// This will complete the background potion of navigations 1 and 2.
	std::async(std::launch::async,
		[this] { m_manualExecutorBackground->loop(std::numeric_limits<size_t>::max()); })
		.wait();

	// This will cause navigations 1 and 2 to be finalized.
	m_manualExecutorCurrent->loop(std::numeric_limits<size_t>::max());

	m_shellEnumerator->SetShouldSucceed(false);
	m_navigationManager->StartNavigation(navigateParams3);

	// This will complete the background potion of navigation 3.
	std::async(std::launch::async,
		[this] { m_manualExecutorBackground->loop(std::numeric_limits<size_t>::max()); })
		.wait();

	// This will cause navigation 3 to be finalized.
	m_manualExecutorCurrent->loop(std::numeric_limits<size_t>::max());
}

TEST_F(NavigationManagerTest, PendingNavigations)
{
	EXPECT_EQ(m_navigationManager->GetNumPendingNavigations(), 0);
	EXPECT_FALSE(m_navigationManager->HasAnyPendingNavigations());

	PidlAbsolute pidl1 = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams1 = NavigateParams::Normal(pidl1.Raw());
	m_navigationManager->StartNavigation(navigateParams1);
	EXPECT_EQ(m_navigationManager->GetNumPendingNavigations(), 1);
	EXPECT_TRUE(m_navigationManager->HasAnyPendingNavigations());

	PidlAbsolute pidl2 = CreateSimplePidlForTest(L"d:\\");
	auto navigateParams2 = NavigateParams::Normal(pidl2.Raw());
	m_navigationManager->StartNavigation(navigateParams2);
	EXPECT_EQ(m_navigationManager->GetNumPendingNavigations(), 2);
	EXPECT_TRUE(m_navigationManager->HasAnyPendingNavigations());

	// This should allow the first navigation to complete.
	m_manualExecutorBackground->loop(1);
	m_manualExecutorCurrent->loop(1);
	EXPECT_EQ(m_navigationManager->GetNumPendingNavigations(), 1);
	EXPECT_TRUE(m_navigationManager->HasAnyPendingNavigations());

	// This should allow the second navigation to complete.
	m_manualExecutorBackground->loop(1);
	m_manualExecutorCurrent->loop(1);
	EXPECT_EQ(m_navigationManager->GetNumPendingNavigations(), 0);
	EXPECT_FALSE(m_navigationManager->HasAnyPendingNavigations());
}
