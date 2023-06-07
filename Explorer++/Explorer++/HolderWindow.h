// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Literals.h"
#include <wil/resource.h>
#include <functional>
#include <optional>

class CoreInterface;

class HolderWindow
{
public:
	using ResizedCallback = std::function<void(int newWidth)>;
	using CloseButtonClickedCallback = std::function<void()>;

	static HolderWindow *Create(HWND parent, const std::wstring &caption, DWORD style,
		const std::wstring &closeButtonTooltip, CoreInterface *coreInterface);

	HWND GetHWND() const;
	void SetResizedCallback(ResizedCallback callback);
	void SetCloseButtonClickedCallback(CloseButtonClickedCallback callback);

private:
	static constexpr WCHAR CLASS_NAME[] = L"Holder";

	static constexpr int CLOSE_BUTTON_ID = 1;

	// A resize can be started if the cursor is this many pixels from the right edge of the window.
	static constexpr int RESIZE_START_RANGE = 6_px;

	HolderWindow(HWND parent, const std::wstring &caption, DWORD style,
		const std::wstring &closeButtonTooltip, CoreInterface *coreInterface);
	HWND CreateHolderWindow(HWND parent, const std::wstring &caption, DWORD style);
	static ATOM RegisterHolderWindowClass();

	static LRESULT CALLBACK WndProcStub(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void OnPaint();
	void OnPrintClient(HDC hdc);
	void PerformPaint(const PAINTSTRUCT &ps);
	void OnSize(int width, int height);
	void OnLButtonDown(const POINT &pt);
	void OnLButtonUp();
	int OnMouseMove(const POINT &pt);
	bool OnSetCursor(HWND target);
	bool IsCursorInResizeStartRange(const POINT &ptCursor);

	const HWND m_hwnd;
	wil::unique_hfont m_font;

	HWND m_toolbar;
	wil::unique_himagelist m_toolbarImageList;

	HCURSOR m_sizingCursor;
	bool m_resizing = false;
	std::optional<int> m_resizeDistanceToEdge;
	ResizedCallback m_resizedCallback;

	CloseButtonClickedCallback m_closeButtonClickedCallback;
};
