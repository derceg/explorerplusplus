// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PreviewPane.h"
#include "Config.h"
#include "DarkModeManager.h"
#include "MainResource.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowSubclass.h"
#include <glog/logging.h>
#include <Shlwapi.h>
#include <propkey.h>

PreviewPane *PreviewPane::Create(HWND parent, const Config *config)
{
	HWND hwnd = CreatePreviewWindow(parent);
	if (!hwnd)
	{
		return nullptr;
	}
	return new PreviewPane(hwnd, config);
}

PreviewPane::PreviewPane(HWND hwnd, const Config *config) :
	m_hwnd(hwnd),
	m_config(config),
	m_backgroundBrush(CreateSolidBrush(DEFAULT_BACKGROUND_COLOR))
{
	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(m_hwnd,
		std::bind_front(&PreviewPane::PreviewWindowProc, this)));
}

PreviewPane::~PreviewPane()
{
	ReleasePreviewHandler();
}

HWND PreviewPane::CreatePreviewWindow(HWND parent)
{
	static bool classRegistered = false;

	if (!classRegistered)
	{
		auto res = RegisterPreviewWindowClass();
		if (!res)
		{
			LOG(ERROR) << "Failed to register PreviewPane window class";
			return nullptr;
		}

		classRegistered = true;
	}

	HWND previewWindow = CreateWindow(CLASS_NAME, L"", WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS, 0,
		0, 0, 0, parent, nullptr, GetModuleHandle(nullptr), nullptr);
	if (!previewWindow)
	{
		LOG(ERROR) << "Failed to create PreviewPane window";
	}

	return previewWindow;
}

ATOM PreviewPane::RegisterPreviewWindowClass()
{
	WNDCLASS windowClass = {};
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = DefWindowProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandle(nullptr);
	windowClass.hIcon = nullptr;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.hbrBackground = nullptr;
	windowClass.lpszMenuName = nullptr;
	windowClass.lpszClassName = CLASS_NAME;
	return RegisterClass(&windowClass);
}

HWND PreviewPane::GetHWND() const
{
	return m_hwnd;
}

LRESULT PreviewPane::PreviewWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
		OnSize(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_PAINT:
		OnPaint();
		return 0;

	case WM_ERASEBKGND:
		return 1;

	case WM_NCDESTROY:
		delete this;
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void PreviewPane::OnSize(int width, int height)
{
	if (m_previewHandler && m_hasPreview)
	{
		RECT rect = { 0, 0, width, height };
		m_previewHandler->SetRect(&rect);
	}
}

void PreviewPane::OnPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(m_hwnd, &ps);

	RECT rect;
	GetClientRect(m_hwnd, &rect);

	if (!m_hasPreview)
	{
		DrawNoPreviewMessage(hdc, rect);
	}

	EndPaint(m_hwnd, &ps);
}

void PreviewPane::DrawNoPreviewMessage(HDC hdc, const RECT &rect)
{
	COLORREF bgColor = DEFAULT_BACKGROUND_COLOR;
	COLORREF textColor = RGB(128, 128, 128);

	if (m_config->theme.get() == +Theme::Dark)
	{
		bgColor = DARK_MODE_BACKGROUND_COLOR;
		textColor = RGB(180, 180, 180);
	}

	HBRUSH bgBrush = CreateSolidBrush(bgColor);
	FillRect(hdc, &rect, bgBrush);
	DeleteObject(bgBrush);

	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, textColor);

	const wchar_t *message = L"No preview available";

	if (m_currentFilePath.empty())
	{
		message = L"Select a file to preview";
	}
	else if (!m_hasPreview)
	{
		// Show file name when preview handler is not available
		message = L"Preview not available for this file type";
	}

	DrawText(hdc, message, -1, const_cast<RECT *>(&rect),
		DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

void PreviewPane::SetPreviewFile(PCIDLIST_ABSOLUTE pidl)
{
	OutputDebugString(L"[PreviewPane] SetPreviewFile called\n");

	// Prevent re-entrant calls during initialization
	if (m_isInitializing)
	{
		OutputDebugString(L"[PreviewPane] already initializing, skipping\n");
		return;
	}

	if (!pidl)
	{
		OutputDebugString(L"[PreviewPane] pidl is null, clearing\n");
		ClearPreview();
		return;
	}

	// Get the file path
	std::wstring filePath;
	HRESULT hr = GetDisplayName(pidl, SHGDN_FORPARSING, filePath);

	if (FAILED(hr))
	{
		OutputDebugString(L"[PreviewPane] GetDisplayName failed\n");
		ClearPreview();
		return;
	}

	wchar_t msg[512];
	swprintf_s(msg, L"[PreviewPane] filePath=%s\n", filePath.c_str());
	OutputDebugString(msg);

	// Check if it's a folder - don't preview folders
	SFGAOF attributes = SFGAO_FOLDER;
	hr = GetItemAttributes(pidl, &attributes);

	if (SUCCEEDED(hr) && (attributes & SFGAO_FOLDER))
	{
		OutputDebugString(L"[PreviewPane] item is a folder, clearing\n");
		ClearPreview();
		return;
	}

	// Check if this is the same file (whether preview succeeded or not)
	if (m_currentFilePath == filePath)
	{
		OutputDebugString(L"[PreviewPane] same file, skipping\n");
		return;
	}

	// Set initializing flag
	m_isInitializing = true;

	// Release the current preview handler
	ReleasePreviewHandler();

	m_currentFilePath = filePath;
	m_currentPidl = pidl;

	// Try to initialize the preview handler
	OutputDebugString(L"[PreviewPane] calling InitializePreviewHandler\n");
	if (InitializePreviewHandler(pidl))
	{
		OutputDebugString(L"[PreviewPane] preview handler initialized successfully\n");
		m_hasPreview = true;
	}
	else
	{
		OutputDebugString(L"[PreviewPane] failed to initialize preview handler\n");
		m_hasPreview = false;
	}

	m_isInitializing = false;
	InvalidateRect(m_hwnd, nullptr, TRUE);
}

void PreviewPane::ClearPreview()
{
	ReleasePreviewHandler();

	m_currentFilePath.clear();
	m_currentPidl.Reset();
	m_hasPreview = false;

	InvalidateRect(m_hwnd, nullptr, TRUE);
}

bool PreviewPane::HasPreview() const
{
	return m_hasPreview;
}

bool PreviewPane::InitializePreviewHandler(PCIDLIST_ABSOLUTE pidl)
{
	// Get the file extension
	std::wstring extension = PathFindExtension(m_currentFilePath.c_str());

	if (extension.empty())
	{
		OutputDebugString(L"[PreviewPane] InitializePreviewHandler: empty extension\n");
		return false;
	}

	wchar_t msg[512];
	swprintf_s(msg, L"[PreviewPane] InitializePreviewHandler: extension=%s\n", extension.c_str());
	OutputDebugString(msg);

	// Skip PDF files - Adobe preview handler is incompatible
	if (_wcsicmp(extension.c_str(), L".pdf") == 0)
	{
		OutputDebugString(L"[PreviewPane] PDF preview disabled\n");
		return false;
	}

	// Get the preview handler CLSID for this extension
	CLSID clsid;
	if (!GetPreviewHandlerCLSID(extension, clsid))
	{
		OutputDebugString(L"[PreviewPane] no preview handler found for extension\n");
		return false;
	}

	wchar_t clsidString[64];
	StringFromGUID2(clsid, clsidString, 64);
	swprintf_s(msg, L"[PreviewPane] found CLSID: %s\n", clsidString);
	OutputDebugString(msg);

	// Method 1: Try to get preview handler directly from IShellItem (most reliable)
	// BHID_Preview GUID: {8895b1c6-b41f-4c1c-a562-0d564250836f}
	static const GUID BHID_Preview_Local = { 0x8895b1c6, 0xb41f, 0x4c1c, { 0xa5, 0x62, 0x0d, 0x56, 0x42, 0x50, 0x83, 0x6f } };

	wil::com_ptr_nothrow<IShellItem> shellItem;
	HRESULT hr = SHCreateItemFromParsingName(m_currentFilePath.c_str(), nullptr, IID_PPV_ARGS(&shellItem));

	if (SUCCEEDED(hr))
	{
		hr = shellItem->BindToHandler(nullptr, BHID_Preview_Local, IID_PPV_ARGS(&m_previewHandler));
		if (SUCCEEDED(hr))
		{
			OutputDebugString(L"[PreviewPane] preview handler created via IShellItem::BindToHandler\n");
		}
	}

	// Method 2: Fallback to CoCreateInstance
	if (!m_previewHandler)
	{
		hr = CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_previewHandler));

		if (FAILED(hr))
		{
			hr = CoCreateInstance(clsid, nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&m_previewHandler));
		}

		if (FAILED(hr))
		{
			hr = CoCreateInstance(clsid, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&m_previewHandler));
		}

		if (SUCCEEDED(hr))
		{
			OutputDebugString(L"[PreviewPane] preview handler created via CoCreateInstance\n");
		}
	}

	if (!m_previewHandler)
	{
		swprintf_s(msg, L"[PreviewPane] Failed to create preview handler, hr=0x%08X\n", hr);
		OutputDebugString(msg);
		return false;
	}

	// Try to initialize - try IInitializeWithFile first (Adobe PDF prefers this)
	bool initialized = false;

	// Method 1: IInitializeWithFile (preferred for Adobe PDF)
	wil::com_ptr_nothrow<IInitializeWithFile> initWithFile;
	hr = m_previewHandler->QueryInterface(IID_PPV_ARGS(&initWithFile));

	if (SUCCEEDED(hr))
	{
		OutputDebugString(L"[PreviewPane] Trying IInitializeWithFile\n");
		hr = initWithFile->Initialize(m_currentFilePath.c_str(), STGM_READ);
		if (SUCCEEDED(hr))
		{
			OutputDebugString(L"[PreviewPane] Initialized with IInitializeWithFile\n");
			initialized = true;
		}
		else
		{
			swprintf_s(msg, L"[PreviewPane] IInitializeWithFile::Initialize failed, hr=0x%08X\n", hr);
			OutputDebugString(msg);
		}
	}
	else
	{
		OutputDebugString(L"[PreviewPane] IInitializeWithFile not supported\n");
	}

	// Method 2: IInitializeWithStream
	if (!initialized)
	{
		wil::com_ptr_nothrow<IInitializeWithStream> initWithStream;
		hr = m_previewHandler->QueryInterface(IID_PPV_ARGS(&initWithStream));

		if (SUCCEEDED(hr))
		{
			OutputDebugString(L"[PreviewPane] Trying IInitializeWithStream\n");
			m_previewStream.reset();
			// Use STGM_SHARE_DENY_WRITE for better compatibility with Adobe PDF
			hr = SHCreateStreamOnFileEx(m_currentFilePath.c_str(), STGM_READ | STGM_SHARE_DENY_WRITE,
				0, FALSE, nullptr, &m_previewStream);

			if (SUCCEEDED(hr))
			{
				hr = initWithStream->Initialize(m_previewStream.get(), STGM_READ);
				if (SUCCEEDED(hr))
				{
					OutputDebugString(L"[PreviewPane] Initialized with IInitializeWithStream\n");
					initialized = true;
				}
			}
		}
	}

	// Method 3: IInitializeWithItem
	if (!initialized)
	{
		wil::com_ptr_nothrow<IInitializeWithItem> initWithItem;
		hr = m_previewHandler->QueryInterface(IID_PPV_ARGS(&initWithItem));

		if (SUCCEEDED(hr))
		{
			OutputDebugString(L"[PreviewPane] Trying IInitializeWithItem\n");
			wil::com_ptr_nothrow<IShellItem> itemShellItem;
			hr = SHCreateItemFromIDList(pidl, IID_PPV_ARGS(&itemShellItem));

			if (SUCCEEDED(hr))
			{
				hr = initWithItem->Initialize(itemShellItem.get(), STGM_READ);
				if (SUCCEEDED(hr))
				{
					OutputDebugString(L"[PreviewPane] Initialized with IInitializeWithItem\n");
					initialized = true;
				}
			}
		}
	}

	if (!initialized)
	{
		OutputDebugString(L"[PreviewPane] Failed to initialize preview handler\n");
		m_previewHandler.reset();
		return false;
	}

	// Set the preview handler frame (required by some handlers like Adobe PDF)
	wil::com_ptr_nothrow<IObjectWithSite> objectWithSite;
	hr = m_previewHandler->QueryInterface(IID_PPV_ARGS(&objectWithSite));
	if (SUCCEEDED(hr))
	{
		objectWithSite->SetSite(static_cast<IPreviewHandlerFrame *>(this));
		OutputDebugString(L"[PreviewPane] Set IPreviewHandlerFrame site\n");
	}

	// Set the window - ensure we have a valid size
	RECT rect;
	GetClientRect(m_hwnd, &rect);
	
	// Ensure minimum size to avoid issues with some preview handlers
	if (rect.right - rect.left < 10 || rect.bottom - rect.top < 10)
	{
		rect.right = rect.left + 300;
		rect.bottom = rect.top + 300;
	}
	
	swprintf_s(msg, L"[PreviewPane] SetWindow rect=(%d,%d,%d,%d)\n", rect.left, rect.top, rect.right, rect.bottom);
	OutputDebugString(msg);

	hr = m_previewHandler->SetWindow(m_hwnd, &rect);

	if (FAILED(hr))
	{
		swprintf_s(msg, L"[PreviewPane] SetWindow failed, hr=0x%08X\n", hr);
		OutputDebugString(msg);
		m_previewHandler.reset();
		return false;
	}

	// Try to set the background color
	m_previewHandler->QueryInterface(IID_PPV_ARGS(&m_previewHandlerVisuals));

	if (m_previewHandlerVisuals)
	{
		COLORREF bgColor = (m_config->theme.get() == +Theme::Dark) ? DARK_MODE_BACKGROUND_COLOR
																   : DEFAULT_BACKGROUND_COLOR;
		m_previewHandlerVisuals->SetBackgroundColor(bgColor);
	}

	// Process pending messages before DoPreview - some handlers need this
	MSG winMsg;
	while (PeekMessage(&winMsg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&winMsg);
		DispatchMessage(&winMsg);
	}

	// Show the preview
	OutputDebugString(L"[PreviewPane] calling DoPreview\n");
	hr = m_previewHandler->DoPreview();

	if (FAILED(hr))
	{
		swprintf_s(msg, L"[PreviewPane] DoPreview failed, hr=0x%08X\n", hr);
		OutputDebugString(msg);
		m_previewHandler.reset();
		m_previewHandlerVisuals.reset();
		return false;
	}

	OutputDebugString(L"[PreviewPane] preview shown successfully!\n");
	return true;
}

void PreviewPane::ReleasePreviewHandler()
{
	if (m_previewHandler)
	{
		m_previewHandler->Unload();
		m_previewHandler.reset();
	}

	m_previewHandlerVisuals.reset();
	m_previewStream.reset();
}

bool PreviewPane::GetPreviewHandlerCLSID(const std::wstring &extension, CLSID &clsid)
{
	if (extension.empty())
	{
		return false;
	}

	// Use AssocQueryString - the most reliable method
	wchar_t clsidStr[64] = {};
	DWORD size = sizeof(clsidStr);
	HRESULT hr = AssocQueryString(ASSOCF_INIT_DEFAULTTOSTAR, ASSOCSTR_SHELLEXTENSION,
		extension.c_str(), L"{8895b1c6-b41f-4c1c-a562-0d564250836f}", clsidStr, &size);

	if (SUCCEEDED(hr) && clsidStr[0] != L'\0')
	{
		hr = CLSIDFromString(clsidStr, &clsid);
		if (SUCCEEDED(hr))
		{
			OutputDebugString(L"[PreviewPane] Found CLSID via AssocQueryString\n");
			return true;
		}
	}

	// Fallback: Try direct registry lookup
	std::wstring keyPath = extension + L"\\shellex\\{8895b1c6-b41f-4c1c-a562-0d564250836f}";

	HKEY hKey;
	LONG result = RegOpenKeyEx(HKEY_CLASSES_ROOT, keyPath.c_str(), 0, KEY_READ, &hKey);

	if (result == ERROR_SUCCESS)
	{
		size = sizeof(clsidStr);
		result = RegQueryValueEx(hKey, nullptr, nullptr, nullptr, reinterpret_cast<LPBYTE>(clsidStr),
			&size);
		RegCloseKey(hKey);

		if (result == ERROR_SUCCESS)
		{
			hr = CLSIDFromString(clsidStr, &clsid);
			if (SUCCEEDED(hr))
			{
				OutputDebugString(L"[PreviewPane] Found CLSID via direct registry\n");
				return true;
			}
		}
	}

	// Try SystemFileAssociations with PerceivedType
	wchar_t perceivedType[64] = {};
	DWORD perceivedSize = sizeof(perceivedType);
	result = RegGetValue(HKEY_CLASSES_ROOT, extension.c_str(), L"PerceivedType", RRF_RT_REG_SZ,
		nullptr, perceivedType, &perceivedSize);

	if (result == ERROR_SUCCESS && perceivedType[0] != L'\0')
	{
		keyPath = L"SystemFileAssociations\\" + std::wstring(perceivedType) +
			L"\\shellex\\{8895b1c6-b41f-4c1c-a562-0d564250836f}";

		result = RegOpenKeyEx(HKEY_CLASSES_ROOT, keyPath.c_str(), 0, KEY_READ, &hKey);

		if (result == ERROR_SUCCESS)
		{
			size = sizeof(clsidStr);
			result = RegQueryValueEx(hKey, nullptr, nullptr, nullptr,
				reinterpret_cast<LPBYTE>(clsidStr), &size);
			RegCloseKey(hKey);

			if (result == ERROR_SUCCESS)
			{
				hr = CLSIDFromString(clsidStr, &clsid);
				if (SUCCEEDED(hr))
				{
					OutputDebugString(L"[PreviewPane] Found CLSID via PerceivedType\n");
					return true;
				}
			}
		}
	}

	OutputDebugString(L"[PreviewPane] No preview handler CLSID found in any location\n");
	return false;
}

void PreviewPane::OnConfigChanged()
{
	// Update background color based on theme
	if (m_previewHandlerVisuals)
	{
		COLORREF bgColor = (m_config->theme.get() == +Theme::Dark) ? DARK_MODE_BACKGROUND_COLOR
																   : DEFAULT_BACKGROUND_COLOR;
		m_previewHandlerVisuals->SetBackgroundColor(bgColor);
	}

	InvalidateRect(m_hwnd, nullptr, TRUE);
}

// IUnknown implementation
STDMETHODIMP PreviewPane::QueryInterface(REFIID riid, void **ppv)
{
	if (!ppv)
	{
		return E_POINTER;
	}

	if (riid == IID_IUnknown || riid == IID_IPreviewHandlerFrame)
	{
		*ppv = static_cast<IPreviewHandlerFrame *>(this);
		AddRef();
		return S_OK;
	}

	*ppv = nullptr;
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) PreviewPane::AddRef()
{
	return InterlockedIncrement(&m_refCount);
}

STDMETHODIMP_(ULONG) PreviewPane::Release()
{
	ULONG count = InterlockedDecrement(&m_refCount);
	// Note: Don't delete this - PreviewPane lifetime is managed externally
	return count;
}

// IPreviewHandlerFrame implementation
STDMETHODIMP PreviewPane::GetWindowContext(PREVIEWHANDLERFRAMEINFO *pinfo)
{
	if (!pinfo)
	{
		return E_POINTER;
	}

	pinfo->haccel = nullptr;
	pinfo->cAccelEntries = 0;
	return S_OK;
}

STDMETHODIMP PreviewPane::TranslateAccelerator(MSG *pmsg)
{
	UNREFERENCED_PARAMETER(pmsg);
	return S_FALSE;  // We don't handle accelerators
}
