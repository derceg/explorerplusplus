// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/StringHelper.h"
#include <memory>
#include <string>

struct Config;
class CoreInterface;
class ResizableDialogHelper;
class ResourceLoader;

class OptionsPage
{
public:
	using SettingChangedCallback = std::function<void()>;

	OptionsPage(UINT dialogResourceId, UINT titleResourceId, HWND parent,
		const ResourceLoader *resourceLoader, HINSTANCE resourceInstance, Config *config,
		CoreInterface *coreInterface, SettingChangedCallback settingChangedCallback,
		HWND tooltipWindow);
	virtual ~OptionsPage();

	void InitializeDialog();
	HWND GetDialog() const;
	std::wstring GetTitle() const;
	bool DoesPageContainText(const std::wstring &text, StringComparatorFunc stringComparator);

	virtual void SaveSettings() = 0;

protected:
	Config *const m_config;
	CoreInterface *const m_coreInterface;
	const ResourceLoader *const m_resourceLoader;
	const HINSTANCE m_resourceInstance;
	const HWND m_tooltipWindow;

	const SettingChangedCallback m_settingChangedCallback;

private:
	struct TextSearchData
	{
		const std::wstring text;
		const StringComparatorFunc stringComparator;
		bool textFound = false;

		TextSearchData(const std::wstring &text, StringComparatorFunc stringComparator) :
			text(text),
			stringComparator(stringComparator)
		{
		}
	};

	static INT_PTR CALLBACK DialogProcStub(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK DialogProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam);

	virtual std::unique_ptr<ResizableDialogHelper> InitializeResizeDialogHelper() = 0;
	virtual void InitializeControls() = 0;

	virtual void OnCommand(WPARAM wParam, LPARAM lParam);
	virtual INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);

	static BOOL CALLBACK CheckChildWindowForTextMatch(HWND hwnd, LPARAM lParam);

	virtual INT_PTR DialogProcExtra(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND m_dialog = nullptr;
	const UINT m_dialogResourceId;
	const UINT m_titleResourceId;
	const HWND m_parent;
	std::unique_ptr<ResizableDialogHelper> m_resizableDialogHelper;
};
