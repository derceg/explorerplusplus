// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ProcessManager.h"
#include "BrowserList.h"
#include "BrowserWindow.h"
#include "CommandLine.h"
#include "Config.h"
#include "TestHelper.h"
#include "Version.h"
#include "VersionHelper.h"
#include "../Helper/MessageWindowHelper.h"
#include "../Helper/WindowSubclass.h"

ProcessManager::ProcessManager(const BrowserList *browserList) : m_browserList(browserList)
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
			AttemptToNotifyExistingProcess(existingWindow, commandLineSettings->directories);
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
	auto *browser = m_browserList->GetLastActive();

	if (!browser)
	{
		return;
	}

	if (cds->cbData > 0)
	{
		std::wstring directory(static_cast<wchar_t *>(cds->lpData), cds->cbData / sizeof(wchar_t));

		browser->OpenItem(directory, OpenFolderDisposition::NewTabDefault);
	}
	else
	{
		browser->OpenDefaultItem(OpenFolderDisposition::NewTabDefault);
	}

	browser->Activate();
}

void ProcessManager::AttemptToNotifyExistingProcess(HWND existingWindow,
	const std::vector<std::wstring> &directories)
{
	DWORD processId;
	auto threadId = GetWindowThreadProcessId(existingWindow, &processId);

	if (threadId == 0)
	{
		return;
	}

	AllowSetForegroundWindow(processId);

	if (!directories.empty())
	{
		for (const auto &directory : directories)
		{
			COPYDATASTRUCT cds = {};
			cds.cbData = static_cast<DWORD>(directory.size() * sizeof(wchar_t));
			cds.lpData = const_cast<wchar_t *>(directory.c_str());
			SendMessage(existingWindow, WM_COPYDATA, NULL, reinterpret_cast<LPARAM>(&cds));
		}
	}
	else
	{
		COPYDATASTRUCT cds = {};
		SendMessage(existingWindow, WM_COPYDATA, NULL, reinterpret_cast<LPARAM>(&cds));
	}
}
