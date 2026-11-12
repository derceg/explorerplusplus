// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ProcessManager.h"
#include "BrowserList.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "BrowserWindowMock.h"
#include "CommandLine.h"
#include "Config.h"
#include "MessageLoop.h"
#include "PidlTestHelper.h"
#include "RuntimeTestHelper.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/NavigationRequest.h"
#include "ShellNameParserFake.h"
#include "../Helper/Helper.h"
#include <gtest/gtest.h>

using namespace testing;

class ProcessManagerTest : public Test
{
protected:
	struct ProcessData
	{
		BrowserList browserList;
		ShellNameParserFake shellNameParser;
		Runtime runtime;
		ProcessManager processManager;
		CommandLine::Settings settings;
		Config config;

		ProcessData() :
			runtime(BuildRuntimeForTest()),
			processManager(&browserList, &shellNameParser, &runtime)
		{
		}

		bool Initialize(const std::wstring &windowName = CreateUniqueWindowName())
		{
			auto result = processManager.InitializeCurrentProcess(&settings, &config, windowName);
			MessageLoop messageLoop;
			messageLoop.RunUntilIdle();
			return result;
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

class ProcessManagerFileSelectionTest : public BrowserTestBase
{
protected:
	ShellNameParserFake m_shellNameParser;
};

TEST_F(ProcessManagerFileSelectionTest, ExistingProcessSelectFilesInForegroundTabs)
{
	ProcessManager receiver(&m_browserList, &m_shellNameParser, &m_runtime);
	auto windowName = CreateGUID();
	ASSERT_TRUE(receiver.InitializeCurrentProcess(&m_commandLineSettings, &m_config, windowName));
	auto *browser = AddBrowser();
	m_config.openTabsInForeground = false;

	std::vector<std::wstring> files = { L"c:\\project with spaces\\\u6587\u4ef6.cpp",
		L"d:\\other\\second.h", L"relative folder\\third.cpp" };
	std::vector<NavigateParams> requests;
	boost::signals2::scoped_connection connection =
		m_navigationEvents.AddStartedObserver([&](const NavigationRequest *request)
			{ requests.push_back(request->GetNavigateParams()); }, NavigationEventScope::Global());

	CommandLine::Settings settings;
	settings.filesToSelect = files;
	Config senderConfig;
	senderConfig.allowMultipleInstances = false;
	ProcessManager sender(&m_browserList, &m_shellNameParser, &m_runtime);
	EXPECT_FALSE(sender.InitializeCurrentProcess(&settings, &senderConfig, windowName));
	EXPECT_TRUE(requests.empty());
	MessageLoop messageLoop;
	messageLoop.RunUntilIdle();
	ASSERT_EQ(requests.size(), files.size());

	for (size_t i = 0; i < files.size(); ++i)
	{
		auto file = CreateSimplePidlForTest(std::filesystem::absolute(files[i]).wstring());
		auto parent = file;
		parent.RemoveLastItem();
		EXPECT_EQ(requests[i].pidl, parent);
		EXPECT_EQ(requests[i].itemsToSelect, std::vector<PidlAbsolute>{ file });
	}
	EXPECT_EQ(browser->GetActiveTabBar()->GetSelectedTabIndex(),
		browser->GetActiveTabBar()->GetNumTabs() - 1);
}

TEST_F(ProcessManagerFileSelectionTest, ReceiverDestroyedBeforePendingRequest)
{
	auto receiver =
		std::make_unique<ProcessManager>(&m_browserList, &m_shellNameParser, &m_runtime);
	auto windowName = CreateGUID();
	ASSERT_TRUE(receiver->InitializeCurrentProcess(&m_commandLineSettings, &m_config, windowName));
	auto *browser = AddBrowser();
	int initialTabCount = browser->GetActiveTabBar()->GetNumTabs();

	CommandLine::Settings settings;
	settings.filesToSelect = { L"c:\\project\\file.cpp" };
	Config senderConfig;
	senderConfig.allowMultipleInstances = false;
	ProcessManager sender(&m_browserList, &m_shellNameParser, &m_runtime);
	EXPECT_FALSE(sender.InitializeCurrentProcess(&settings, &senderConfig, windowName));
	receiver.reset();

	MessageLoop messageLoop;
	messageLoop.RunUntilIdle();
	EXPECT_EQ(browser->GetActiveTabBar()->GetNumTabs(), initialTabCount);
}
