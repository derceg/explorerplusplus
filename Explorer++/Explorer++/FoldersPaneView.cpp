// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FoldersPaneView.h"
#include "MainFontSetter.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "SystemFontHelper.h"
#include "ThemeManager.h"
#include "../Helper/DoubleBufferedPaint.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclass.h"
#include <windowsx.h>

FoldersPaneView::FoldersPaneView(ViewBase *parent, MenuHelpTextHost *menuHelpTextHost,
	const ThemeManager *themeManager, const Config *config, const ResourceLoader *resourceLoader) :
	ViewBase(CreateFoldersPane(parent)),
	m_toolbarView(this, menuHelpTextHost, config),
	m_foldersText(resourceLoader->LoadString(IDS_FOLDERS_WINDOW_TEXT)),
	m_themeManager(themeManager),
	m_sizingCursor(LoadCursor(nullptr, IDC_SIZEWE))
{
	AddSubclass(std::make_unique<WindowSubclass>(GetHWND(),
		std::bind_front(&FoldersPaneView::WndProc, this)));

	LOGFONT systemFont = GetDefaultSystemFontScaledToWindow(GetHWND());
	m_defaultFont.reset(CreateFontIndirect(&systemFont));
	CHECK(m_defaultFont);

	m_font = m_defaultFont.get();

	m_fontSetter = std::make_unique<MainFontSetter>(GetHWND(), config);

	m_toolbarView.AddSizeRequirementsChangedObserver([this] { OnCaptionMetricsChanged(); });
}

HWND FoldersPaneView::CreateFoldersPane(ViewBase *parent)
{
	static bool classRegistered = false;

	if (!classRegistered)
	{
		auto res = RegisterFoldersPaneClass();
		CHECK_NE(res, 0);

		classRegistered = true;
	}

	HWND hwnd = CreateWindowEx(WS_EX_CONTROLPARENT, CLASS_NAME, L"",
		WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, 0, 0, parent->GetHWND(), nullptr,
		GetModuleHandle(nullptr), nullptr);
	CHECK(hwnd);
	return hwnd;
}

ATOM FoldersPaneView::RegisterFoldersPaneClass()
{
	WNDCLASS windowClass = {};
	windowClass.lpfnWndProc = DefWindowProc;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.lpszClassName = CLASS_NAME;
	windowClass.hInstance = GetModuleHandle(nullptr);
	windowClass.style = CS_DBLCLKS;
	return RegisterClass(&windowClass);
}

FoldersPaneToolbarView *FoldersPaneView::GetToolbarView()
{
	return &m_toolbarView;
}

LRESULT FoldersPaneView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_LBUTTONDOWN:
		OnLButtonDown({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
		break;

	case WM_LBUTTONUP:
		ReleaseCapture();
		break;

	case WM_MOUSEMOVE:
		return OnMouseMove({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });

	case WM_CAPTURECHANGED:
		OnCaptureChanged(reinterpret_cast<HWND>(lParam));
		break;

	case WM_SETCURSOR:
		if (OnSetCursor(reinterpret_cast<HWND>(wParam)))
		{
			return TRUE;
		}
		break;

	case WM_SETFONT:
		OnSetFont(reinterpret_cast<HFONT>(wParam), lParam);
		break;

	case WM_DPICHANGED_AFTERPARENT:
		OnCaptionMetricsChanged();
		break;

	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT:
		OnPaint();
		return 0;

	// The toolbar that's contained within this window has a transparent background, with the
	// background from the parent being drawn underneath it. For that background to be correctly
	// drawn, WM_PRINTCLIENT needs to be handled.
	case WM_PRINTCLIENT:
		OnPrintClient(reinterpret_cast<HDC>(wParam));
		return 0;

	case WM_SIZE:
		OnSize(LOWORD(lParam), HIWORD(lParam));
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void FoldersPaneView::OnSetFont(HFONT font, bool redraw)
{
	SetFont(font);

	if (redraw)
	{
		RedrawWindow(GetHWND(), nullptr, nullptr, RDW_UPDATENOW);
	}
}

void FoldersPaneView::SetFont(HFONT font)
{
	if (font)
	{
		m_font = font;
	}
	else
	{
		m_font = m_defaultFont.get();
	}

	OnCaptionMetricsChanged();
}

void FoldersPaneView::OnCaptionMetricsChanged()
{
	m_captionSectionHeight.reset();
	UpdateLayout();
	InvalidateRect(GetHWND(), nullptr, false);
}

void FoldersPaneView::OnPaint()
{
	DoubleBufferedPaint doubleBufferedPaint(GetHWND());
	Paint(doubleBufferedPaint.GetMemoryDC(), doubleBufferedPaint.GetPaintRect());
}

void FoldersPaneView::OnPrintClient(HDC hdc)
{
	RECT clientRect;
	auto res = GetClientRect(GetHWND(), &clientRect);
	CHECK(res);
	Paint(hdc, clientRect);
}

void FoldersPaneView::Paint(HDC hdc, const RECT &paintRect)
{
	auto *colorProvider = m_themeManager->GetCurrentColorProvider();
	FillRect(hdc, &paintRect, colorProvider->GetTabBarBackgroundBrush());

	auto selectFont = wil::SelectObject(hdc, m_font);
	SetBkMode(hdc, TRANSPARENT);

	SetTextColor(hdc, colorProvider->GetTextColor());

	RECT toolbarRect;
	GetWindowRect(m_toolbarView.GetHWND(), &toolbarRect);
	MapWindowPoints(HWND_DESKTOP, GetHWND(), reinterpret_cast<LPPOINT>(&toolbarRect), 2);

	int captionHorizontalPadding = DpiCompatibility::GetInstance().ScaleValue(
		m_toolbarView.GetHWND(), CAPTION_SECTION_HORIZONTAL_PADDING);
	RECT textRect = { captionHorizontalPadding, 0, toolbarRect.left, GetCaptionSectionHeight() };
	DrawText(hdc, m_foldersText.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

void FoldersPaneView::OnSize(int width, int height)
{
	UpdateLayout(width, height);
}

void FoldersPaneView::UpdateLayout()
{
	RECT rc;
	GetClientRect(GetHWND(), &rc);
	UpdateLayout(GetRectWidth(&rc), GetRectHeight(&rc));
}

void FoldersPaneView::UpdateLayout(int width, int height)
{
	auto deferInfo = BeginDeferWindowPos(2);

	auto &dpiCompatibility = DpiCompatibility::GetInstance();
	int captionHorizontalPadding =
		dpiCompatibility.ScaleValue(GetHWND(), CAPTION_SECTION_HORIZONTAL_PADDING);
	int captionSectionHeight = GetCaptionSectionHeight();

	auto toolbarWidth = m_toolbarView.MaybeGetIdealWidth();
	CHECK(toolbarWidth);

	auto toolbarHeight = m_toolbarView.GetHeight();

	deferInfo = DeferWindowPos(deferInfo, m_toolbarView.GetHWND(), nullptr,
		width - *toolbarWidth - captionHorizontalPadding,
		(captionSectionHeight - toolbarHeight) / 2, *toolbarWidth, toolbarHeight, SWP_NOZORDER);

	if (m_treeView)
	{
		auto contentHorizontalPadding =
			dpiCompatibility.ScaleValue(GetHWND(), CONTENT_SECTION_RIGHT_PADDING);
		deferInfo = DeferWindowPos(deferInfo, m_treeView, nullptr, 0, captionSectionHeight,
			width - contentHorizontalPadding, height - captionSectionHeight, SWP_NOZORDER);
	}

	auto res = EndDeferWindowPos(deferInfo);
	CHECK(res);
}

int FoldersPaneView::GetCaptionSectionHeight()
{
	if (!m_captionSectionHeight)
	{
		m_captionSectionHeight = CalculateCaptionSectionHeight();
	}

	return *m_captionSectionHeight;
}

int FoldersPaneView::CalculateCaptionSectionHeight()
{
	SIZE textSize;
	auto hdc = wil::GetDC(GetHWND());
	auto selectFont = wil::SelectObject(hdc.get(), m_font);
	auto textExtentRes = GetTextExtentPoint32(hdc.get(), m_foldersText.c_str(),
		static_cast<int>(m_foldersText.size()), &textSize);
	CHECK(textExtentRes);

	auto verticalPadding =
		DpiCompatibility::GetInstance().ScaleValue(GetHWND(), CAPTION_SECTION_VERTICAL_PADDING);
	return std::max(static_cast<int>(textSize.cy), m_toolbarView.GetHeight())
		+ (verticalPadding * 2);
}

void FoldersPaneView::OnLButtonDown(const POINT &pt)
{
	if (IsCursorInResizeStartRange(pt))
	{
		m_resizing = true;

		RECT clientRect;
		GetClientRect(GetHWND(), &clientRect);
		m_resizeDistanceToEdge = clientRect.right - pt.x;

		SetCapture(GetHWND());
	}
}

int FoldersPaneView::OnMouseMove(const POINT &pt)
{
	if (m_resizing)
	{
		int newWidth = std::max(pt.x + m_resizeDistanceToEdge.value(), 0L);

		if (m_resizedCallback)
		{
			m_resizedCallback(newWidth);
		}

		return 1;
	}

	return 0;
}

void FoldersPaneView::OnCaptureChanged(HWND target)
{
	if (target != GetHWND())
	{
		m_resizing = false;
		m_resizeDistanceToEdge.reset();
	}
}

bool FoldersPaneView::OnSetCursor(HWND target)
{
	if (target != GetHWND())
	{
		return false;
	}

	DWORD cursorPos = GetMessagePos();
	POINT ptCursor = { GET_X_LPARAM(cursorPos), GET_Y_LPARAM(cursorPos) };
	ScreenToClient(GetHWND(), &ptCursor);

	if (IsCursorInResizeStartRange(ptCursor))
	{
		// Without this, the cursor would switch back and forth between the sizing cursor and arrow
		// cursor when near the right edge of the window.
		SetCursor(m_sizingCursor);
		return true;
	}

	return false;
}

bool FoldersPaneView::IsCursorInResizeStartRange(const POINT &ptCursor)
{
	RECT clientRect;
	GetClientRect(GetHWND(), &clientRect);

	InflateRect(&clientRect,
		-DpiCompatibility::GetInstance().ScaleValue(GetHWND(), RESIZE_START_RANGE), 0);

	return ptCursor.x >= clientRect.right;
}

void FoldersPaneView::SetTreeView(HWND treeView)
{
	DCHECK(GetParent(treeView) == GetHWND());
	m_treeView = treeView;
	UpdateLayout();
}

void FoldersPaneView::SetResizedCallback(ResizedCallback callback)
{
	m_resizedCallback = callback;
}
