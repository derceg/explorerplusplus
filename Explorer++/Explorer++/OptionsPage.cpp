// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "OptionsPage.h"
#include "ResourceLoader.h"
#include "../Helper/Controls.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/ResizableDialogHelper.h"
#include "../Helper/WindowHelper.h"
#include <memory>
#include <unordered_set>

OptionsPage::OptionsPage(UINT dialogResourceId, UINT titleResourceId, HWND parent,
	const ResourceLoader *resourceLoader, HINSTANCE resourceInstance, Config *config,
	CoreInterface *coreInterface, SettingChangedCallback settingChangedCallback,
	HWND tooltipWindow) :
	m_config(config),
	m_coreInterface(coreInterface),
	m_resourceLoader(resourceLoader),
	m_resourceInstance(resourceInstance),
	m_tooltipWindow(tooltipWindow),
	m_settingChangedCallback(settingChangedCallback),
	m_dialogResourceId(dialogResourceId),
	m_titleResourceId(titleResourceId),
	m_parent(parent)
{
}

OptionsPage::~OptionsPage() = default;

// Actual dialog creation is a separate method, since it's not possible to create the dialog in
// the constructor for this class. The reason being that the handler for WM_INITDIALOG will
// attempt to call virtual methods, which will fail if the derived class hasn't been fully
// instantiated yet.
void OptionsPage::InitializeDialog()
{
	if (m_dialog)
	{
		DCHECK(false);
		return;
	}

	m_resourceLoader->CreateModelessDialog(m_dialogResourceId, m_parent, DialogProcStub,
		reinterpret_cast<LPARAM>(this));
}

HWND OptionsPage::GetDialog() const
{
	return m_dialog;
}

std::wstring OptionsPage::GetTitle() const
{
	return m_resourceLoader->LoadString(m_titleResourceId);
}

INT_PTR CALLBACK OptionsPage::DialogProcStub(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	auto *optionsDialogPage = reinterpret_cast<OptionsPage *>(GetWindowLongPtr(dlg, GWLP_USERDATA));

	switch (msg)
	{
	case WM_INITDIALOG:
	{
		optionsDialogPage = reinterpret_cast<OptionsPage *>(lParam);

		SetLastError(0);
		auto res =
			SetWindowLongPtr(dlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(optionsDialogPage));
		DCHECK(!(res == 0 && GetLastError() != 0));
	}
	break;

	case WM_NCDESTROY:
		SetWindowLongPtr(dlg, GWLP_USERDATA, 0);
		return 0;
	}

	if (!optionsDialogPage)
	{
		return 0;
	}

	return optionsDialogPage->DialogProc(dlg, msg, wParam, lParam);
}

INT_PTR CALLBACK OptionsPage::DialogProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		m_dialog = dlg;

		m_resizableDialogHelper = InitializeResizeDialogHelper();
		InitializeControls();
	}
	break;

	case WM_COMMAND:
		OnCommand(wParam, lParam);
		break;

	case WM_NOTIFY:
		return OnNotify(wParam, lParam);

	case WM_SIZE:
		m_resizableDialogHelper->UpdateControls(LOWORD(lParam), HIWORD(lParam));
		return 0;
	}

	return DialogProcExtra(dlg, msg, wParam, lParam);
}

INT_PTR OptionsPage::DialogProcExtra(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(dlg);
	UNREFERENCED_PARAMETER(msg);
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	return 0;
}

void OptionsPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);
}

INT_PTR OptionsPage::OnNotify(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	return 0;
}

bool OptionsPage::DoesPageContainText(const std::wstring &text,
	StringComparatorFunc stringComparator)
{
	TextSearchData searchData(text, stringComparator);
	EnumChildWindows(m_dialog, CheckChildWindowForTextMatch, reinterpret_cast<LPARAM>(&searchData));
	return searchData.textFound;
}

BOOL CALLBACK OptionsPage::CheckChildWindowForTextMatch(HWND hwnd, LPARAM lParam)
{
	WCHAR className[256];
	auto res = GetClassName(hwnd, className, static_cast<int>(std::size(className)));

	if (res == 0)
	{
		DCHECK(false);
		return TRUE;
	}

	auto *searchData = reinterpret_cast<TextSearchData *>(lParam);
	bool matchFound = false;

	if (lstrcmp(className, WC_BUTTON) == 0 || lstrcmp(className, WC_STATIC) == 0)
	{
		auto windowText = GetWindowString(hwnd);
		matchFound = searchData->stringComparator(windowText, searchData->text);
	}
	else if (lstrcmp(className, WC_COMBOBOX) == 0)
	{
		matchFound = DoesComboBoxContainText(hwnd, searchData->text, searchData->stringComparator);
	}
	else if (lstrcmp(className, WC_LISTVIEW) == 0)
	{
		matchFound = ListViewHelper::DoesListViewContainText(hwnd, searchData->text,
			searchData->stringComparator);
	}

	if (matchFound)
	{
		searchData->textFound = true;

		// The text has been found, so there's no need to keep searching.
		return FALSE;
	}

	return TRUE;
}
