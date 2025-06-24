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

class DriveModelTest : public Test
{
protected:
	DriveModelTest() :
		m_drives({ L"C:\\", L"D:\\", L"E:\\" }),
		m_driveModel(std::make_unique<DriveEnumeratorFake>(m_drives), &m_driveWatcher)
	{
		m_driveModel.AddDriveAddedObserver(m_driveAddedCallback.AsStdFunction());
		m_driveModel.AddDriveUpdatedObserver(m_driveUpdatedCallback.AsStdFunction());
		m_driveModel.AddDriveRemovedObserver(m_driveRemovedCallback.AsStdFunction());
	}

	std::set<std::wstring> m_drives;
	DriveWatcherFake m_driveWatcher;
	MockFunction<void(const std::wstring &path, size_t index)> m_driveAddedCallback;
	MockFunction<void(const std::wstring &path)> m_driveUpdatedCallback;
	MockFunction<void(const std::wstring &path, size_t oldIndex)> m_driveRemovedCallback;
	DriveModel m_driveModel;
};

TEST_F(DriveModelTest, AddDrive)
{
	std::wstring pathToAdd = L"F:\\";
	EXPECT_CALL(m_driveAddedCallback, Call(pathToAdd, 3));
	m_driveWatcher.AddDrive(pathToAdd);

	m_drives.emplace(pathToAdd);
	EXPECT_EQ(m_driveModel.GetDrives(), m_drives);
}

TEST_F(DriveModelTest, RemoveDrive)
{
	std::wstring pathToRemove = L"D:\\";
	EXPECT_CALL(m_driveRemovedCallback, Call(pathToRemove, 1));
	m_driveWatcher.RemoveDrive(pathToRemove);

	m_drives.erase(pathToRemove);
	EXPECT_EQ(m_driveModel.GetDrives(), m_drives);
}

TEST_F(DriveModelTest, AddDuplicateDrive)
{
	std::wstring pathToAdd = L"D:\\";
	EXPECT_CALL(m_driveAddedCallback, Call(_, _)).Times(0);
	m_driveWatcher.AddDrive(pathToAdd);
	EXPECT_EQ(m_driveModel.GetDrives(), m_drives);
}

TEST_F(DriveModelTest, RemoveNonExistentDrive)
{
	std::wstring pathToRemove = L"F:\\";
	EXPECT_CALL(m_driveRemovedCallback, Call(_, _)).Times(0);
	m_driveWatcher.RemoveDrive(pathToRemove);
	EXPECT_EQ(m_driveModel.GetDrives(), m_drives);
}

TEST_F(DriveModelTest, UpdateDrive)
{
	std::wstring path = L"C:\\";
	EXPECT_CALL(m_driveUpdatedCallback, Call(path));
	m_driveWatcher.UpdateDrive(path);
}

TEST_F(DriveModelTest, GetDriveIndex)
{
	EXPECT_EQ(m_driveModel.GetDriveIndex(L"C:\\"), 0u);
	EXPECT_EQ(m_driveModel.GetDriveIndex(L"D:\\"), 1u);
	EXPECT_EQ(m_driveModel.GetDriveIndex(L"E:\\"), 2u);
}
