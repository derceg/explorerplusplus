// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Application.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Applications;
using namespace testing;

class ApplicationObserverMock
{
public:
	ApplicationObserverMock(Application *application)
	{
		application->AddUpdatedObserver(
			std::bind_front(&ApplicationObserverMock::OnApplicationUpdated, this));
	}

	MOCK_METHOD(void, OnApplicationUpdated, (Application * application));
};

class ApplicationTest : public Test
{
protected:
	ApplicationTest() :
		m_application(L"notepad", L"C:\\Windows\\System32\\notepad.exe"),
		m_observer(&m_application)
	{
	}

	Application m_application;
	ApplicationObserverMock m_observer;
};

TEST_F(ApplicationTest, Update)
{
	EXPECT_CALL(m_observer, OnApplicationUpdated(&m_application));
	m_application.SetName(L"cmd");

	EXPECT_CALL(m_observer, OnApplicationUpdated(&m_application));
	m_application.SetCommand(L"C:\\Windows\\system32\\cmd.exe");

	EXPECT_CALL(m_observer, OnApplicationUpdated(&m_application));
	m_application.SetShowNameOnToolbar(!m_application.GetShowNameOnToolbar());
}

TEST_F(ApplicationTest, UpdateWithoutChange)
{
	// No observers should be invoked if the application details haven't actually changed.
	EXPECT_CALL(m_observer, OnApplicationUpdated(&m_application)).Times(0);
	m_application.SetName(m_application.GetName());

	EXPECT_CALL(m_observer, OnApplicationUpdated(&m_application)).Times(0);
	m_application.SetCommand(m_application.GetCommand());

	EXPECT_CALL(m_observer, OnApplicationUpdated(&m_application)).Times(0);
	m_application.SetShowNameOnToolbar(m_application.GetShowNameOnToolbar());
}
