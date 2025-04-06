// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "DriveModel.h"
#include "DriveEnumeratorFake.h"
#include "DriveWatcherFake.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <set>
#include <string>

using namespace testing;

class DriveModelObserverMock
{
public:
	DriveModelObserverMock(DriveModel *driveModel)
	{
		driveModel->AddDriveAddedObserver(
			std::bind_front(&DriveModelObserverMock::OnDriveAdded, this));
		driveModel->AddDriveUpdatedObserver(
			std::bind_front(&DriveModelObserverMock::OnDriveUpdated, this));
		driveModel->AddDriveRemovedObserver(
			std::bind_front(&DriveModelObserverMock::OnDriveRemoved, this));
	}

	MOCK_METHOD(void, OnDriveAdded, (const std::wstring &path, size_t index));
	MOCK_METHOD(void, OnDriveUpdated, (const std::wstring &path));
	MOCK_METHOD(void, OnDriveRemoved, (const std::wstring &path, size_t oldIndex));
};

class DriveModelTest : public Test
{
protected:
	DriveModelTest() :
		m_drives({ L"C:\\", L"D:\\", L"E:\\" }),
		m_driveModel(std::make_unique<DriveEnumeratorFake>(m_drives), &m_driveWatcher),
		m_observerMock(&m_driveModel)
	{
	}

	std::set<std::wstring> m_drives;
	DriveWatcherFake m_driveWatcher;
	DriveModel m_driveModel;
	DriveModelObserverMock m_observerMock;
};

TEST_F(DriveModelTest, AddDrive)
{
	std::wstring pathToAdd = L"F:\\";

	EXPECT_CALL(m_observerMock, OnDriveAdded(pathToAdd, 3));

	m_driveWatcher.AddDrive(pathToAdd);

	m_drives.emplace(pathToAdd);
	EXPECT_EQ(m_driveModel.GetDrives(), m_drives);
}

TEST_F(DriveModelTest, RemoveDrive)
{
	std::wstring pathToRemove = L"D:\\";

	EXPECT_CALL(m_observerMock, OnDriveRemoved(pathToRemove, 1));

	m_driveWatcher.RemoveDrive(pathToRemove);

	m_drives.erase(pathToRemove);
	EXPECT_EQ(m_driveModel.GetDrives(), m_drives);
}

TEST_F(DriveModelTest, AddDuplicateDrive)
{
	std::wstring pathToAdd = L"D:\\";

	EXPECT_CALL(m_observerMock, OnDriveAdded(_, _)).Times(0);

	m_driveWatcher.AddDrive(pathToAdd);

	EXPECT_EQ(m_driveModel.GetDrives(), m_drives);
}

TEST_F(DriveModelTest, RemoveNonExistentDrive)
{
	std::wstring pathToRemove = L"F:\\";

	EXPECT_CALL(m_observerMock, OnDriveRemoved(_, _)).Times(0);

	m_driveWatcher.RemoveDrive(pathToRemove);

	EXPECT_EQ(m_driveModel.GetDrives(), m_drives);
}
