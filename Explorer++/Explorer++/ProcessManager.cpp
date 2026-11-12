// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ProcessManager.h"
#include "BrowserList.h"
#include "BrowserWindow.h"
#include "CommandLine.h"
#include "Config.h"
#include "Runtime.h"
#include "ShellBrowser/NavigateParams.h"
#include "ShellNameParser.h"
#include "TabBar.h"
#include "TestHelper.h"
#include "Version.h"
#include "VersionHelper.h"
#include "../Helper/MessageWindowHelper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowSubclass.h"

namespace
{
constexpr ULONG_PTR SELECT_FILE = 1;
}

ProcessManager::ProcessManager(const BrowserList *browserList, ShellNameParser *shellNameParser,
	const Runtime *runtime) :
	m_browserList(browserList),
	m_shellNameParser(shellNameParser),
	m_runtime(runtime)
{
}

bool ProcessManager::InitializeCurrentProcess(const CommandLine::Settings *commandLineSettings,
	const Config *config, const std::wstring &overriddenWindowName)
{
	CHECK(!m_initializationRun);

	m_initializationRun = true;

	wil::unique_mutex_nothrow mutex;
	bool res = mutex.try_create(L"Explorer++FindExistingWindow");

	if (!res)
	{
		return false;
	}

	auto lock = mutex.acquire();

	// Using the version number as the window name will mean that when attempting to find an
	// existing window, only a window that's created by the current version will match. Running
	// different versions simultaneously isn't a supported scenario, so the fact that
	// allowMultipleInstances won't work in that situation isn't a bug.
	auto windowName = VersionHelper::GetVersion().GetString();

	if (!overriddenWindowName.empty())
	{
		CHECK(IsInTest());
		windowName = overriddenWindowName;
	}

	HWND existingWindow = FindWindowEx(HWND_MESSAGE, nullptr,
		MessageWindowHelper::MESSAGE_CLASS_NAME, windowName.c_str());

	if (existingWindow)
	{
		if (commandLineSettings->jumplistNewTab)
		{
			AttemptToNotifyExistingProcess(existingWindow);
			return false;
		}

		if (!config->allowMultipleInstances)
		{
			AttemptToNotifyExistingProcess(existingWindow, commandLineSettings->directories,
				commandLineSettings->filesToSelect);
			return false;
		}
	}

	m_messageWindow = MessageWindowHelper::CreateMessageOnlyWindow(windowName);
	m_messageWindowSubclass = std::make_unique<WindowSubclass>(m_messageWindow.get(),
		std::bind_front(&ProcessManager::MessageWindowProc, this));

	return true;
}

LRESULT ProcessManager::MessageWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_COPYDATA:
		OnCopyData(reinterpret_cast<COPYDATASTRUCT *>(lParam));
		return TRUE;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void ProcessManager::OnCopyData(const COPYDATASTRUCT *cds)
{
	if (cds->dwData != 0 && cds->dwData != SELECT_FILE)
	{
		return;
	}

	std::wstring path;
	if (cds->cbData > 0)
	{
		if (!cds->lpData || cds->cbData % sizeof(wchar_t) != 0)
		{
			return;
		}

		path.assign(static_cast<const wchar_t *>(cds->lpData), cds->cbData / sizeof(wchar_t));
	}

	// Copy the payload before returning, then leave SendMessage's input-synchronous context
	// before creating tabs and calling the shell's out-of-process COM interfaces.
	m_runtime->GetUiThreadExecutor()->post(
		[weakSelf = m_weakPtrFactory.GetWeakPtr(), path = std::move(path), type = cds->dwData]
		{
			if (weakSelf)
			{
				weakSelf->OpenItem(path, type);
			}
		});
}

void ProcessManager::OpenItem(const std::wstring &path, ULONG_PTR type)
{
	auto *browser = m_browserList->GetLastActive();
	if (!browser)
	{
		return;
	}

	if (!path.empty())
	{
		if (type == SELECT_FILE)
		{
			auto result = m_shellNameParser->ParseDisplayName(path);
			if (!result)
			{
				return;
			}

			auto parent = result.value();
			parent.RemoveLastItem();
			auto navigateParams = NavigateParams::Normal(parent.Raw());
			navigateParams.itemsToSelect = { result.value() };
			browser->GetActiveTabBar()->CreateNewTab(navigateParams, { .selected = true });
		}
		else
		{
			browser->OpenItem(path, OpenFolderDisposition::NewTabDefault);
		}
	}
	else
	{
		browser->OpenDefaultItem(OpenFolderDisposition::NewTabDefault);
	}

	browser->Activate();
}

void ProcessManager::AttemptToNotifyExistingProcess(HWND existingWindow,
	const std::vector<std::wstring> &directories, const std::vector<std::wstring> &filesToSelect)
{
	DWORD processId;
	auto threadId = GetWindowThreadProcessId(existingWindow, &processId);

	if (threadId == 0)
	{
		return;
	}

	AllowSetForegroundWindow(processId);

	if (!directories.empty() || !filesToSelect.empty())
	{
		auto currentDirectory = GetCurrentDirectoryWrapper();
		if (!currentDirectory)
		{
			return;
		}

		// Resolve paths in the sending process; the existing window can have a different cwd.
		auto sendPath = [&](const std::wstring &path, ULONG_PTR type)
		{
			auto absolutePath = TransformUserEnteredPathToAbsolutePathAndNormalize(path,
				*currentDirectory, EnvVarsExpansion::DontExpand);
			if (!absolutePath)
			{
				return;
			}

			COPYDATASTRUCT cds = {};
			cds.dwData = type;
			cds.cbData = static_cast<DWORD>(absolutePath->size() * sizeof(wchar_t));
			cds.lpData = const_cast<wchar_t *>(absolutePath->c_str());
			SendMessage(existingWindow, WM_COPYDATA, NULL, reinterpret_cast<LPARAM>(&cds));
		};

		for (const auto &directory : directories)
		{
			sendPath(directory, 0);
		}

		for (const auto &file : filesToSelect)
		{
			sendPath(file, SELECT_FILE);
		}
	}
	else
	{
		COPYDATASTRUCT cds = {};
		SendMessage(existingWindow, WM_COPYDATA, NULL, reinterpret_cast<LPARAM>(&cds));
	}
}
