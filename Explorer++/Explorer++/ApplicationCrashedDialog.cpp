// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ApplicationCrashedDialog.h"
#include "Explorer++_internal.h"
#include "../Helper/ShellHelper.h"
#include <format>

ApplicationCrashedDialog::ApplicationCrashedDialog(std::optional<std::wstring> crashDumpFileName) :
	m_crashDumpFileName(crashDumpFileName)
{
}

void ApplicationCrashedDialog::Show()
{
	std::wstring message;
	std::vector<TASKDIALOG_BUTTON> customButtons = { { REPORT_ISSUE_BUTTON_ID,
		L"Report issue..." } };

	if (m_crashDumpFileName)
	{
		message = std::format(L"A crash dump has been saved to:\n\n{}\n\n"
							  L"If you report this crash, please include the crash dump.",
			*m_crashDumpFileName);

		customButtons.push_back({ SHOW_CRASH_DUMP_BUTTON_ID, L"Show crash dump in folder..." });
	}
	else
	{
		message = L"A crash dump could not be created.";
	}

	TASKDIALOGCONFIG dialogConfig = { 0 };
	dialogConfig.cbSize = sizeof(dialogConfig);
	dialogConfig.hwndParent = nullptr;
	dialogConfig.hInstance = GetModuleHandle(nullptr);
	dialogConfig.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_SIZE_TO_CONTENT;
	dialogConfig.dwCommonButtons = TDCBF_CLOSE_BUTTON;
	dialogConfig.pszWindowTitle = NExplorerplusplus::APP_NAME;
	dialogConfig.pszMainIcon = TD_ERROR_ICON;
	dialogConfig.pszMainInstruction = L"Explorer++ has encountered an error.";
	dialogConfig.pszContent = message.c_str();
	dialogConfig.cButtons = static_cast<UINT>(customButtons.size());
	dialogConfig.pButtons = &customButtons[0];
	dialogConfig.nDefaultButton = IDCLOSE;
	dialogConfig.cRadioButtons = 0;
	dialogConfig.pRadioButtons = nullptr;
	dialogConfig.pszVerificationText = nullptr;
	dialogConfig.pszExpandedInformation = nullptr;
	dialogConfig.pszFooter = nullptr;
	dialogConfig.pfCallback = CallbackStub;
	dialogConfig.lpCallbackData = reinterpret_cast<LONG_PTR>(this);
	dialogConfig.cxWidth = 0;
	TaskDialogIndirect(&dialogConfig, nullptr, nullptr, nullptr);
}

HRESULT CALLBACK ApplicationCrashedDialog::CallbackStub(HWND hwnd, UINT msg, WPARAM wParam,
	LPARAM lParam, LONG_PTR data)
{
	auto *dialog = reinterpret_cast<ApplicationCrashedDialog *>(data);
	return dialog->Callback(hwnd, msg, wParam, lParam);
}

HRESULT ApplicationCrashedDialog::Callback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(lParam);

	switch (msg)
	{
	case TDN_BUTTON_CLICKED:
		switch (wParam)
		{
		case REPORT_ISSUE_BUTTON_ID:
			ShellExecute(nullptr, L"open", REPORT_ISSUE_URL, nullptr, nullptr, SW_SHOW);
			return S_FALSE;

		case SHOW_CRASH_DUMP_BUTTON_ID:
		{
			PidlAbsolute pidl;
			HRESULT hr = CreateSimplePidl(m_crashDumpFileName.value(), pidl);

			if (SUCCEEDED(hr))
			{
				SHOpenFolderAndSelectItems(pidl.Raw(), 0, nullptr, 0);
			}
		}
			return S_FALSE;
		}
		break;
	}

	return S_OK;
}
