// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ProcessManager.h"
#include "BrowserList.h"
#include "BrowserWindowMock.h"
#include "CommandLine.h"
#include "Config.h"
#include "../Helper/Helper.h"
#include <gtest/gtest.h>

using namespace testing;

class ProcessManagerTest : public Test
{
protected:
	struct ProcessData
	{
		BrowserList browserList;
		ProcessManager processManager;
		CommandLine::Settings settings;
		Config config;

		ProcessData() : processManager(&browserList)
		{
		}

		bool Initialize(const std::wstring &windowName = CreateUniqueWindowName())
		{
			return processManager.InitializeCurrentProcess(&settings, &config, windowName);
		}
	};

	static std::wstring CreateUniqueWindowName()
	{
		// ProcessManager will create a message-only window. To find an existing process, it will
		// search for an existing message-only window that has the appropriate class name and window
		// name. A random value will be returned here to ensure that each test runs correctly, even
		// if Explorer++, or another test is currently running.
		return CreateGUID();
	}
};

TEST_F(ProcessManagerTest, NoExistingProcess)
{
	// allowMultipleInstances isn't set, but there are no other processes, so this initialization
	// should succeed.
	ProcessData processData;
	processData.config.allowMultipleInstances = false;
	auto res = processData.Initialize();
	EXPECT_TRUE(res);
}

TEST_F(ProcessManagerTest, ExistingProcess)
{
	auto windowName = CreateGUID();

	ProcessData processData1;
	auto res = processData1.Initialize(windowName);
	ASSERT_TRUE(res);

	ProcessData processData2;
	processData2.config.allowMultipleInstances = false;
	res = processData2.Initialize(windowName);
	EXPECT_FALSE(res);
}

TEST_F(ProcessManagerTest, ExistingProcessOpenDefaultDirectory)
{
	auto windowName = CreateGUID();

	ProcessData processData1;
	auto res = processData1.Initialize(windowName);
	ASSERT_TRUE(res);

	BrowserWindowMock browser;
	processData1.browserList.AddBrowser(&browser);

	// There are no directories specified in the command line settings for the second process, so
	// the first process should attempt to open a tab in the default directory.
	EXPECT_CALL(browser, OpenDefaultItem(OpenFolderDisposition::NewTabDefault));
	EXPECT_CALL(browser, Activate());

	ProcessData processData2;
	processData2.config.allowMultipleInstances = false;
	res = processData2.Initialize(windowName);
	EXPECT_FALSE(res);
}

TEST_F(ProcessManagerTest, ExistingProcessOpenDirectories)
{
	auto windowName = CreateGUID();

	ProcessData processData1;
	auto res = processData1.Initialize(windowName);
	ASSERT_TRUE(res);

	std::vector<std::wstring> directories = { L"c:\\", L"c:\\windows", L"c:\\users\\default" };

	BrowserWindowMock browser;
	processData1.browserList.AddBrowser(&browser);

	for (const auto &directory : directories)
	{
		EXPECT_CALL(browser, OpenItem(directory, OpenFolderDisposition::NewTabDefault));
	}

	EXPECT_CALL(browser, Activate()).Times(static_cast<int>(directories.size()));

	ProcessData processData2;
	processData2.settings.directories = directories;
	processData2.config.allowMultipleInstances = false;
	res = processData2.Initialize(windowName);
	EXPECT_FALSE(res);
}
