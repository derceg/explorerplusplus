// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <optional>
#include <string>

class ApplicationCrashedDialog
{
public:
	ApplicationCrashedDialog(std::optional<std::wstring> crashDumpFileName);

	void Show();

private:
	static constexpr int REPORT_ISSUE_BUTTON_ID = 100;
	static constexpr int SHOW_CRASH_DUMP_BUTTON_ID = 101;
	static constexpr WCHAR REPORT_ISSUE_URL[] =
		L"https://github.com/derceg/explorerplusplus/issues/new?labels=bug,crash&template=crash_report.md";

	static HRESULT CALLBACK CallbackStub(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
		LONG_PTR data);
	HRESULT Callback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	std::optional<std::wstring> m_crashDumpFileName;
};
