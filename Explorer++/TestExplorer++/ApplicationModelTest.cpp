// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "ApplicationModel.h"
#include "Application.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Applications;
using namespace testing;

class ApplicationModelObserverMock
{
public:
	ApplicationModelObserverMock(ApplicationModel *model)
	{
		model->AddApplicationAddedObserver(
			std::bind_front(&ApplicationModelObserverMock::OnApplicationAdded, this));
		model->AddApplicationUpdatedObserver(
			std::bind_front(&ApplicationModelObserverMock::OnApplicationUpdated, this));
		model->AddApplicationRemovedObserver(
			std::bind_front(&ApplicationModelObserverMock::OnApplicationRemoved, this));
	}

	MOCK_METHOD(void, OnApplicationAdded, (Application * application, size_t index));
	MOCK_METHOD(void, OnApplicationUpdated, (Application * application));
	MOCK_METHOD(void, OnApplicationRemoved, (const Application *application, size_t oldIndex));
};

class ApplicationModelTest : public Test
{
protected:
	ApplicationModelTest() : m_observer(&m_model)
	{
	}

	std::unique_ptr<Application> MakeDefaultApplication()
	{
		return std::make_unique<Application>(L"notepad", L"C:\\Windows\\System32\\notepad.exe");
	}

	ApplicationModel m_model;
	ApplicationModelObserverMock m_observer;
};

TEST_F(ApplicationModelTest, AddApplication)
{
	auto application = MakeDefaultApplication();
	auto rawApplication = application.get();

	EXPECT_CALL(m_observer, OnApplicationAdded(rawApplication, 0));
	m_model.AddApplication(std::move(application));

	EXPECT_EQ(m_model.GetApplications().size(), 1);
	EXPECT_EQ(m_model.GetApplicationAtIndex(0), rawApplication);
	EXPECT_EQ(m_model.GetApplicationIndex(rawApplication), 0);
}

TEST_F(ApplicationModelTest, UpdateApplication)
{
	auto application = MakeDefaultApplication();
	auto rawApplication = m_model.AddApplication(std::move(application));

	EXPECT_CALL(m_observer, OnApplicationUpdated(rawApplication));
	rawApplication->SetName(L"cmd");

	EXPECT_CALL(m_observer, OnApplicationUpdated(rawApplication));
	rawApplication->SetCommand(L"C:\\Windows\\system32\\cmd.exe");

	EXPECT_CALL(m_observer, OnApplicationUpdated(rawApplication));
	rawApplication->SetShowNameOnToolbar(!rawApplication->GetShowNameOnToolbar());
}

TEST_F(ApplicationModelTest, UpdateApplicationWithoutChange)
{
	auto application = MakeDefaultApplication();
	auto rawApplication = m_model.AddApplication(std::move(application));

	// No observers should be invoked if the application details haven't actually changed.
	EXPECT_CALL(m_observer, OnApplicationUpdated(rawApplication)).Times(0);
	rawApplication->SetName(rawApplication->GetName());

	EXPECT_CALL(m_observer, OnApplicationUpdated(rawApplication)).Times(0);
	rawApplication->SetCommand(rawApplication->GetCommand());

	EXPECT_CALL(m_observer, OnApplicationUpdated(rawApplication)).Times(0);
	rawApplication->SetShowNameOnToolbar(rawApplication->GetShowNameOnToolbar());
}

TEST_F(ApplicationModelTest, RemoveApplication)
{
	auto application = MakeDefaultApplication();
	auto rawApplication = m_model.AddApplication(std::move(application));

	EXPECT_CALL(m_observer, OnApplicationRemoved(rawApplication, 0));
	m_model.RemoveApplication(rawApplication);

	EXPECT_EQ(m_model.GetApplications().size(), 0);
}

TEST_F(ApplicationModelTest, GetNonExistentApplication)
{
	auto application = MakeDefaultApplication();
	auto rawApplication = m_model.AddApplication(std::move(application));

	EXPECT_EQ(m_model.GetApplicationAtIndex(1), nullptr);
}
