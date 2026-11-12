// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/WeakPtrFactory.h"
#include <boost/core/noncopyable.hpp>
#include <wil/resource.h>
#include <memory>

namespace CommandLine
{

struct Settings;

}

class BrowserList;
class Runtime;
class ShellNameParser;
struct Config;
class WindowSubclass;

class ProcessManager : private boost::noncopyable
{
public:
	ProcessManager(const BrowserList *browserList, ShellNameParser *shellNameParser,
		const Runtime *runtime);

	bool InitializeCurrentProcess(const CommandLine::Settings *commandLineSettings,
		const Config *config, const std::wstring &overriddenWindowName = L"");

private:
	LRESULT MessageWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnCopyData(const COPYDATASTRUCT *cds);
	void OpenItem(const std::wstring &path, ULONG_PTR type);

	void AttemptToNotifyExistingProcess(HWND existingWindow,
		const std::vector<std::wstring> &directories = {},
		const std::vector<std::wstring> &filesToSelect = {});

	const BrowserList *const m_browserList;
	ShellNameParser *const m_shellNameParser;
	const Runtime *const m_runtime;
	bool m_initializationRun = false;
	wil::unique_hwnd m_messageWindow;
	std::unique_ptr<WindowSubclass> m_messageWindowSubclass;
	WeakPtrFactory<ProcessManager> m_weakPtrFactory{ this };
};
