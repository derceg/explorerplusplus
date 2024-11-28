// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/core/noncopyable.hpp>
#include <wil/resource.h>
#include <memory>

namespace CommandLine
{

struct Settings;

}

class BrowserList;
struct Config;
class WindowSubclass;

class ProcessManager : private boost::noncopyable
{
public:
	ProcessManager(const BrowserList *browserList);

	bool InitializeCurrentProcess(const CommandLine::Settings *commandLineSettings,
		const Config *config, const std::wstring &overriddenWindowName = L"");

private:
	LRESULT MessageWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnCopyData(const COPYDATASTRUCT *cds);

	void AttemptToNotifyExistingProcess(HWND existingWindow,
		const std::vector<std::wstring> &directories = {});

	const BrowserList *const m_browserList;
	bool m_initializationRun = false;
	wil::unique_hwnd m_messageWindow;
	std::unique_ptr<WindowSubclass> m_messageWindowSubclass;
};
