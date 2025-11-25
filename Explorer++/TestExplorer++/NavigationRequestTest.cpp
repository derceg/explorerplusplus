// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellBrowser/NavigationRequest.h"
#include "NavigationRequestDelegateMock.h"
#include "PidlTestHelper.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/NavigationRequestDelegate.h"
#include "ShellEnumeratorFake.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class NavigationRequestTest : public Test
{
protected:
	NavigationRequestTest() :
		m_shellEnumerator(std::make_shared<ShellEnumeratorFake>()),
		m_manualExecutorBackground(std::make_shared<concurrencpp::manual_executor>()),
		m_manualExecutorCurrent(std::make_shared<concurrencpp::manual_executor>())
	{
	}

	~NavigationRequestTest()
	{
		m_manualExecutorBackground->shutdown();
		m_manualExecutorCurrent->shutdown();
	}

	std::unique_ptr<NavigationRequest> MakeNavigationRequest(const NavigateParams &navigateParams)
	{
		return std::make_unique<NavigationRequest>(nullptr, &m_navigationEvents, GetDelegate(),
			m_shellEnumerator, m_manualExecutorBackground, m_manualExecutorCurrent, navigateParams,
			m_stopSource.get_token());
	}

	std::unique_ptr<NavigationRequest> MakeNavigationRequestForStateTest()
	{
		PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\");
		auto navigateParams = NavigateParams::Normal(pidl.Raw());

		auto request = MakeNavigationRequest(navigateParams);
		request->Start();
		RunExecutors();

		return request;
	}

	virtual NavigationRequestDelegate *GetDelegate()
	{
		return &m_delegate;
	}

	void RunExecutors()
	{
		m_manualExecutorBackground->loop(std::numeric_limits<size_t>::max());
		m_manualExecutorCurrent->loop(std::numeric_limits<size_t>::max());
	}

	NavigationEvents m_navigationEvents;
	const std::shared_ptr<ShellEnumeratorFake> m_shellEnumerator;
	const std::shared_ptr<concurrencpp::manual_executor> m_manualExecutorBackground;
	const std::shared_ptr<concurrencpp::manual_executor> m_manualExecutorCurrent;
	std::stop_source m_stopSource;

private:
	NiceMock<NavigationRequestDelegateMock> m_delegate;
};

TEST_F(NavigationRequestTest, StartedState)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams = NavigateParams::Normal(pidl.Raw());

	auto request = MakeNavigationRequest(navigateParams);
	EXPECT_EQ(request->GetState(), NavigationRequest::State::NotStarted);

	request->Start();
	EXPECT_EQ(request->GetState(), NavigationRequest::State::Started);
}

TEST_F(NavigationRequestTest, CommittedState)
{
	auto request = MakeNavigationRequestForStateTest();
	request->Commit();
	EXPECT_EQ(request->GetState(), NavigationRequest::State::Committed);
}

TEST_F(NavigationRequestTest, FailedState)
{
	auto request = MakeNavigationRequestForStateTest();
	request->Fail();
	EXPECT_EQ(request->GetState(), NavigationRequest::State::Failed);
}

TEST_F(NavigationRequestTest, CancelledState)
{
	auto request = MakeNavigationRequestForStateTest();
	request->Cancel();
	EXPECT_EQ(request->GetState(), NavigationRequest::State::Cancelled);
}

TEST_F(NavigationRequestTest, GetNavigateParams)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams = NavigateParams::Normal(pidl.Raw());

	auto request = MakeNavigationRequest(navigateParams);
	EXPECT_EQ(request->GetNavigateParams(), navigateParams);
}

TEST_F(NavigationRequestTest, Stopped)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams = NavigateParams::Normal(pidl.Raw());

	auto request = MakeNavigationRequest(navigateParams);

	request->Start();
	EXPECT_FALSE(request->Stopped());

	m_stopSource.request_stop();
	EXPECT_TRUE(request->Stopped());
}

class NavigationRequestSignalTest : public NavigationRequestTest
{
protected:
	NavigationRequestSignalTest()
	{
		m_navigationEvents.AddStartedObserver(m_navigationStartedCallback.AsStdFunction(),
			NavigationEventScope::Global());
		m_navigationEvents.AddWillCommitObserver(m_navigationWillCommitCallback.AsStdFunction(),
			NavigationEventScope::Global());
		m_navigationEvents.AddCommittedObserver(m_navigationCommittedCallback.AsStdFunction(),
			NavigationEventScope::Global());
		m_navigationEvents.AddFailedObserver(m_navigationFailedCallback.AsStdFunction(),
			NavigationEventScope::Global());
		m_navigationEvents.AddCancelledObserver(m_navigationCancelledCallback.AsStdFunction(),
			NavigationEventScope::Global());
	}

	NavigationRequestDelegate *GetDelegate() override
	{
		return &m_strictDelegate;
	}

	StrictMock<MockFunction<void(const NavigationRequest *request)>> m_navigationStartedCallback;
	StrictMock<MockFunction<void(const NavigationRequest *request)>> m_navigationWillCommitCallback;
	StrictMock<MockFunction<void(const NavigationRequest *request)>> m_navigationCommittedCallback;
	StrictMock<MockFunction<void(const NavigationRequest *request)>> m_navigationFailedCallback;
	StrictMock<MockFunction<void(const NavigationRequest *request)>> m_navigationCancelledCallback;

	StrictMock<NavigationRequestDelegateMock> m_strictDelegate;
};

TEST_F(NavigationRequestSignalTest, CommittedNavigation)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams = NavigateParams::Normal(pidl.Raw());

	auto request = MakeNavigationRequest(navigateParams);

	{
		InSequence seq;

		EXPECT_CALL(m_navigationStartedCallback, Call(request.get()));
		EXPECT_CALL(m_strictDelegate, OnEnumerationCompleted(request.get()));
		EXPECT_CALL(m_navigationWillCommitCallback, Call(request.get()));
		EXPECT_CALL(m_navigationCommittedCallback, Call(request.get()));
		EXPECT_CALL(m_strictDelegate, OnFinished(request.get()));
	}

	request->Start();
	RunExecutors();

	request->Commit();
}

TEST_F(NavigationRequestSignalTest, FailedNavigation)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams = NavigateParams::Normal(pidl.Raw());

	auto request = MakeNavigationRequest(navigateParams);

	{
		InSequence seq;

		EXPECT_CALL(m_navigationStartedCallback, Call(request.get()));
		EXPECT_CALL(m_strictDelegate, OnEnumerationFailed(request.get()));
		EXPECT_CALL(m_navigationFailedCallback, Call(request.get()));
		EXPECT_CALL(m_strictDelegate, OnFinished(request.get()));
	}

	m_shellEnumerator->SetShouldSucceed(false);
	request->Start();
	RunExecutors();

	request->Fail();
}

TEST_F(NavigationRequestSignalTest, CancelledNavigation)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams = NavigateParams::Normal(pidl.Raw());

	auto request = MakeNavigationRequest(navigateParams);

	{
		InSequence seq;

		EXPECT_CALL(m_navigationStartedCallback, Call(request.get()));
		EXPECT_CALL(m_strictDelegate, OnEnumerationStopped(request.get()));
		EXPECT_CALL(m_navigationCancelledCallback, Call(request.get()));
		EXPECT_CALL(m_strictDelegate, OnFinished(request.get()));
	}

	request->Start();
	m_stopSource.request_stop();
	RunExecutors();

	request->Cancel();
}
