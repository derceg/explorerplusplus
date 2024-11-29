// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Literals.h"
#include <wil/resource.h>
#include <functional>
#include <optional>

struct Config;
class DarkModeHelper;
class IconResourceLoader;
class MainFontSetter;

class HolderWindow
{
public:
	using ResizedCallback = std::function<void(int newWidth)>;
	using CloseButtonClickedCallback = std::function<void()>;

	static HolderWindow *Create(HWND parent, const std::wstring &caption, DWORD style,
		const std::wstring &closeButtonTooltip, const Config *config,
		const IconResourceLoader *iconResourceLoader, const DarkModeHelper *darkModeHelper);

	HWND GetHWND() const;
	void SetContentChild(HWND contentChild);
	void SetResizedCallback(ResizedCallback callback);
	void SetCloseButtonClickedCallback(CloseButtonClickedCallback callback);

private:
	static constexpr WCHAR CLASS_NAME[] = L"Holder";

	static constexpr int CAPTION_SECTION_HORIZONTAL_PADDING = 4_px;
	static constexpr int CAPTION_SECTION_VERTICAL_PADDING = 1_px;

	static constexpr int CONTENT_SECTION_RIGHT_PADDING = 4_px;

	static constexpr int CLOSE_BUTTON_ID = 1;

	// A resize can be started if the cursor is this many pixels from the right edge of the window.
	static constexpr int RESIZE_START_RANGE = 6_px;

	HolderWindow(HWND parent, const std::wstring &caption, DWORD style,
		const std::wstring &closeButtonTooltip, const Config *config,
		const IconResourceLoader *iconResourceLoader, const DarkModeHelper *darkModeHelper);
	HWND CreateHolderWindow(HWND parent, const std::wstring &caption, DWORD style);
	static ATOM RegisterHolderWindowClass();

	static LRESULT CALLBACK WndProcStub(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void OnSetFont(HFONT font, bool redraw);
	void SetFont(HFONT font);
	void OnPaint();
	void OnPrintClient(HDC hdc);
	void PerformPaint(const PAINTSTRUCT &ps);
	void OnSize(int width, int height);
	void UpdateLayout();
	void UpdateLayout(int width, int height);
	int GetCaptionSectionHeight();
	int CalculateCaptionSectionHeight();
	void OnLButtonDown(const POINT &pt);
	void OnLButtonUp();
	int OnMouseMove(const POINT &pt);
	bool OnSetCursor(HWND target);
	bool IsCursorInResizeStartRange(const POINT &ptCursor);

	// In the call to CreateWindowEx(), a WM_SIZE message will be dispatched. At that point, this
	// class won't have been fully initialized, so the WM_SIZE handler shouldn't attempt to access
	// members of the class. To determine whether or not initialization has finished, this variable
	// will be used. Because of that, it's important that it appear first, to ensure that it always
	// has a valid value, regardless of whether or not the constructor has finished running.
	bool m_initialized = false;

	const HWND m_hwnd;
	const DarkModeHelper *const m_darkModeHelper;
	HWND m_contentChild = nullptr;
	HFONT m_font = nullptr;
	wil::unique_hfont m_defaultFont = nullptr;
	std::unique_ptr<MainFontSetter> m_fontSetter;
	std::unique_ptr<MainFontSetter> m_tooltipFontSetter;
	std::optional<int> m_captionSectionHeight;

	HWND m_toolbar;
	wil::unique_himagelist m_toolbarImageList;

	HCURSOR m_sizingCursor;
	bool m_resizing = false;
	std::optional<int> m_resizeDistanceToEdge;
	ResizedCallback m_resizedCallback;

	CloseButtonClickedCallback m_closeButtonClickedCallback;
};
