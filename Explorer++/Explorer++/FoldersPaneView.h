// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "FoldersPaneToolbarView.h"
#include "Literals.h"
#include "ViewBase.h"
#include <wil/resource.h>
#include <functional>
#include <optional>

struct Config;
class MainFontSetter;
class MenuHelpTextHost;
class ResourceLoader;
class ThemeManager;

class FoldersPaneView : public ViewBase
{
public:
	using ResizedCallback = std::function<void(int newWidth)>;

	FoldersPaneView(ViewBase *parent, MenuHelpTextHost *menuHelpTextHost,
		const ThemeManager *themeManager, const Config *config,
		const ResourceLoader *resourceLoader);

	FoldersPaneToolbarView *GetToolbarView();

	void SetTreeView(HWND treeView);
	void SetResizedCallback(ResizedCallback callback);

private:
	static constexpr WCHAR CLASS_NAME[] = L"Explorer++FoldersPaneWindowClass";

	static constexpr int CAPTION_SECTION_HORIZONTAL_PADDING = 4_px;
	static constexpr int CAPTION_SECTION_VERTICAL_PADDING = 1_px;

	static constexpr int CONTENT_SECTION_RIGHT_PADDING = 4_px;

	// A resize can be started if the cursor is this many pixels from the right edge of the window.
	static constexpr int RESIZE_START_RANGE = 6_px;

	static HWND CreateFoldersPane(ViewBase *parent);
	static ATOM RegisterFoldersPaneClass();

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void OnSetFont(HFONT font, bool redraw);
	void SetFont(HFONT font);
	void OnCaptionMetricsChanged();
	void OnPaint();
	void OnPrintClient(HDC hdc);
	void Paint(HDC hdc, const RECT &paintRect);
	void OnSize(int width, int height);
	void UpdateLayout();
	void UpdateLayout(int width, int height);
	int GetCaptionSectionHeight();
	int CalculateCaptionSectionHeight();
	void OnLButtonDown(const POINT &pt);
	int OnMouseMove(const POINT &pt);
	void OnCaptureChanged(HWND target);
	bool OnSetCursor(HWND target);
	bool IsCursorInResizeStartRange(const POINT &ptCursor);

	FoldersPaneToolbarView m_toolbarView;
	const std::wstring m_foldersText;
	const ThemeManager *const m_themeManager;
	HWND m_treeView = nullptr;
	HFONT m_font = nullptr;
	wil::unique_hfont m_defaultFont;
	std::unique_ptr<MainFontSetter> m_fontSetter;
	std::optional<int> m_captionSectionHeight;

	HCURSOR m_sizingCursor;
	bool m_resizing = false;
	std::optional<int> m_resizeDistanceToEdge;
	ResizedCallback m_resizedCallback;
};
