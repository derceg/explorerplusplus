// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ShellHelper.h"
#include "../Helper/WindowSubclass.h"
#include <boost/signals2.hpp>
#include <wil/com.h>
#include <wil/resource.h>
#include <ShObjIdl.h>
#include <memory>
#include <vector>

struct Config;
class WindowSubclass;

// PreviewPane provides a document preview panel that uses Windows Shell preview handlers
// to display previews of various file types (text, images, PDF, Office documents, etc.)
class PreviewPane : public IPreviewHandlerFrame
{
public:
	static PreviewPane *Create(HWND parent, const Config *config);

	HWND GetHWND() const;

	// Updates the preview to show the specified file
	void SetPreviewFile(PCIDLIST_ABSOLUTE pidl);

	// Clears the current preview
	void ClearPreview();

	// Returns true if a preview is currently being displayed
	bool HasPreview() const;

	// IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override;
	STDMETHODIMP_(ULONG) AddRef() override;
	STDMETHODIMP_(ULONG) Release() override;

	// IPreviewHandlerFrame
	STDMETHODIMP GetWindowContext(PREVIEWHANDLERFRAMEINFO *pinfo) override;
	STDMETHODIMP TranslateAccelerator(MSG *pmsg) override;

private:
	static inline const wchar_t CLASS_NAME[] = L"PreviewPane";
	static inline const wchar_t WINDOW_NAME[] = L"PreviewPane";

	static constexpr COLORREF DEFAULT_BACKGROUND_COLOR = RGB(255, 255, 255);
	static constexpr COLORREF DARK_MODE_BACKGROUND_COLOR = RGB(32, 32, 32);

	PreviewPane(HWND hwnd, const Config *config);
	~PreviewPane();

	static HWND CreatePreviewWindow(HWND parent);
	static ATOM RegisterPreviewWindowClass();

	LRESULT PreviewWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void OnSize(int width, int height);
	void OnPaint();

	// Initializes the preview handler for the specified file
	bool InitializePreviewHandler(PCIDLIST_ABSOLUTE pidl);

	// Releases the current preview handler
	void ReleasePreviewHandler();

	// Gets the preview handler CLSID for the specified file extension
	bool GetPreviewHandlerCLSID(const std::wstring &extension, CLSID &clsid);

	// Draws a "no preview available" message
	void DrawNoPreviewMessage(HDC hdc, const RECT &rect);

	void OnConfigChanged();

	const HWND m_hwnd;
	const Config *const m_config;

	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;

	// Current preview handler
	wil::com_ptr_nothrow<IPreviewHandler> m_previewHandler;
	wil::com_ptr_nothrow<IPreviewHandlerVisuals> m_previewHandlerVisuals;
	wil::com_ptr_nothrow<IStream> m_previewStream;  // Keep stream alive for preview handlers that need it

	// Current file being previewed
	PidlAbsolute m_currentPidl;
	std::wstring m_currentFilePath;

	// State
	bool m_hasPreview = false;
	bool m_isInitializing = false;
	ULONG m_refCount = 1;

	// Background brush
	wil::unique_hbrush m_backgroundBrush;
};
