// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "OptionsPage.h"
#include <boost/signals2.hpp>
#include <wil/resource.h>
#include <memory>
#include <vector>

class DarkModeManager;
class StartupMode;
class ThemeManager;
class WindowSubclass;

class StartupOptionsPage : public OptionsPage
{
public:
	StartupOptionsPage(HWND parent, HINSTANCE resourceInstance, Config *config,
		CoreInterface *coreInterface, SettingChangedCallback settingChangedCallback,
		HWND tooltipWindow, DarkModeManager *darkModeManager, ThemeManager *themeManager);

	void SaveSettings() override;

private:
	enum class MoveDirection
	{
		Up,
		Down
	};

	// When the startup folders listview is disabled, this color will be used as the listview
	// background color.
	static constexpr COLORREF LISTVIEW_DARK_MODE_DISABLED_BACKGROUND_COLOR = RGB(48, 48, 48);

	std::unique_ptr<ResizableDialogHelper> InitializeResizeDialogHelper() override;

	void InitializeControls() override;
	void SetUpListView();
	int MapStartupModeToControlId(StartupMode startupMode);
	void AddStartupFolders();
	void AddStartupFolder(int index, const std::wstring &startupFolder);

	void OnDarkModeStatusChanged();
	void UpdateListViewTransparentStyle();

	INT_PTR DialogProcExtra(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam) override;
	void OnEraseDialogBackground(HDC hdc);

	LRESULT ListViewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnNotify(WPARAM wParam, LPARAM lParam) override;
	void OnListViewDoubleClick(const NMITEMACTIVATE *itemActivate);
	void OnListViewEndLabelEdit(const NMLVDISPINFO *dispInfo);
	INT_PTR OnListViewKeyDown(const NMLVKEYDOWN *keyDown);
	void MoveStartupFolder(MoveDirection direction);
	void OnListViewDeletePressed();

	DarkModeManager *const m_darkModeManager;
	ThemeManager *const m_themeManager;
	const wil::unique_hbrush m_listViewDisabledBackgroundBrush;
	std::vector<boost::signals2::scoped_connection> m_connections;
	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
};
