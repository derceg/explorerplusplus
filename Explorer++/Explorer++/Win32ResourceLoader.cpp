// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Win32ResourceLoader.h"
#include "DarkModeManager.h"
#include "IconMappings.h"
#include "ResourceHelper.h"
#include "ThemeWindowTracker.h"
#include "../Helper/Helper.h"
#include "../Helper/ImageHelper.h"
#include "../Helper/ImageOperations.h"
#include "../Helper/ResourceHelper.h"

Win32ResourceLoader::Win32ResourceLoader(HINSTANCE resourceInstance, IconSet iconSet,
	const DarkModeManager *darkModeManager, WindowThemeManager *windowThemeManager) :
	m_resourceInstance(resourceInstance),
	m_iconSet(iconSet),
	m_darkModeManager(darkModeManager),
	m_windowThemeManager(windowThemeManager)
{
}

std::wstring Win32ResourceLoader::LoadString(UINT stringId) const
{
	// TODO: The implementation for ResourceHelper::LoadString should be moved into this method,
	// once all calls to it have been migrated to this method.
	return ResourceHelper::LoadString(m_resourceInstance, stringId);
}

wil::com_ptr_nothrow<IWICBitmapSource> Win32ResourceLoader::LoadBitmapForDpi(Icon icon,
	int iconWidth, int iconHeight, int dpi) const
{
	int scaledIconWidth = MulDiv(iconWidth, dpi, USER_DEFAULT_SCREEN_DPI);
	int scaledIconHeight = MulDiv(iconHeight, dpi, USER_DEFAULT_SCREEN_DPI);
	return LoadBitmap(icon, scaledIconWidth, scaledIconHeight);
}

// Loads and scales a PNG resource and then inverts the colors, when required by the current color
// mode.
wil::com_ptr_nothrow<IWICBitmapSource> Win32ResourceLoader::LoadBitmap(Icon icon, int iconWidth,
	int iconHeight) const
{
	auto bitmap = LoadOriginalBitmap(icon, iconWidth, iconHeight);

	if (m_iconSet == IconSet::Color || !m_darkModeManager->IsDarkModeEnabled())
	{
		return bitmap;
	}

	wil::com_ptr_nothrow<IWICBitmapSource> invertedBitmap;
	HRESULT hr = ImageHelper::InvertBitmap(bitmap.get(), invertedBitmap);
	CHECK(SUCCEEDED(hr));

	return invertedBitmap;
}

// This function is based on the steps performed by
// https://docs.microsoft.com/en-us/windows/win32/api/commctrl/nf-commctrl-loadiconmetric when
// loading an icon (see the remarks section on that page for details).
wil::com_ptr_nothrow<IWICBitmapSource> Win32ResourceLoader::LoadOriginalBitmap(Icon icon,
	int iconWidth, int iconHeight) const
{
	const IconMapping *mapping = nullptr;

	switch (m_iconSet)
	{
	case IconSet::Color:
		mapping = &ICON_RESOURCE_MAPPINGS_COLOR;
		break;

	case IconSet::FluentUi:
		mapping = &ICON_RESOURCE_MAPPINGS_FLUENT_UI;
		break;

	case IconSet::Windows10:
		mapping = &ICON_RESOURCE_MAPPINGS_WINDOWS_10;
		break;
	}

	auto itr = mapping->find(icon);
	CHECK(itr != mapping->end());
	const auto &iconSizeMappings = itr->second;

	auto match = std::find_if(iconSizeMappings.begin(), iconSizeMappings.end(),
		[iconWidth, iconHeight](const auto &entry)
		{ return iconWidth <= entry.first && iconHeight <= entry.first; });

	if (match == iconSizeMappings.end())
	{
		match = std::prev(iconSizeMappings.end());
	}

	// Images are always loaded from the executable's resource instance.
	wil::com_ptr_nothrow<IWICBitmapSource> bitmap;
	HRESULT hr = ImageHelper::LoadBitmapFromPNG(GetModuleHandle(nullptr), match->second, bitmap);
	CHECK(SUCCEEDED(hr));

	// If the icon size matches exactly, it doesn't need to be scaled, so can be returned
	// immediately.
	if (match->first == iconWidth && match->first == iconHeight)
	{
		return bitmap;
	}

	wil::com_ptr_nothrow<IWICBitmapSource> scaledBitmap;
	hr = ImageOperations::ResizeBitmap(bitmap.get(), iconWidth, iconHeight,
		WICBitmapInterpolationModeFant, scaledBitmap);
	CHECK(SUCCEEDED(hr));

	return scaledBitmap;
}

INT_PTR Win32ResourceLoader::CreateModalDialog(UINT dialogId, HWND parent,
	DialogProc dialogProc) const
{
	INT_PTR res;

	auto *themedDialogWrapper = ThemedDialogWrapper::Create(dialogProc, m_windowThemeManager);

	if (IsProcessRTL())
	{
		// The only legitimate reason this might fail is allocation failure. Given the small amount
		// of memory involved, that's not very likely to actually happen. If this function does
		// fail, there's also not any good way of handling it. Therefore, the result is simply
		// CHECK'd, which will terminate the application if the call fails.
		auto dialogTemplateEx = MakeRTLDialogTemplate(m_resourceInstance, dialogId);
		CHECK(dialogTemplateEx);

		res = DialogBoxIndirectParam(m_resourceInstance,
			reinterpret_cast<DLGTEMPLATE *>(dialogTemplateEx.get()), parent,
			ThemedDialogWrapper::ThemedDialogProcStub,
			reinterpret_cast<LPARAM>(themedDialogWrapper));
	}
	else
	{
		res = DialogBoxParam(m_resourceInstance, MAKEINTRESOURCE(dialogId), parent,
			ThemedDialogWrapper::ThemedDialogProcStub,
			reinterpret_cast<LPARAM>(themedDialogWrapper));
	}

	if (res == -1)
	{
		LOG_SYSRESULT(GetLastError());
		CHECK(false);
	}

	return res;
}

HWND Win32ResourceLoader::CreateModelessDialog(UINT dialogId, HWND parent,
	DialogProc dialogProc) const
{
	HWND dialog;

	auto *themedDialogWrapper = ThemedDialogWrapper::Create(dialogProc, m_windowThemeManager);

	if (IsProcessRTL())
	{
		auto dialogTemplateEx = MakeRTLDialogTemplate(m_resourceInstance, dialogId);
		CHECK(dialogTemplateEx);

		dialog = CreateDialogIndirectParam(m_resourceInstance,
			reinterpret_cast<DLGTEMPLATE *>(dialogTemplateEx.get()), parent,
			ThemedDialogWrapper::ThemedDialogProcStub,
			reinterpret_cast<LPARAM>(themedDialogWrapper));
	}
	else
	{
		dialog = CreateDialogParam(m_resourceInstance, MAKEINTRESOURCE(dialogId), parent,
			ThemedDialogWrapper::ThemedDialogProcStub,
			reinterpret_cast<LPARAM>(themedDialogWrapper));
	}

	if (!dialog)
	{
		LOG_SYSRESULT(GetLastError());
		CHECK(false);
	}

	return dialog;
}

Win32ResourceLoader::Win32ResourceLoader::ThemedDialogWrapper *Win32ResourceLoader::
	ThemedDialogWrapper::Create(DialogProc originalDialogProc,
		WindowThemeManager *windowThemeManager)
{
	return new ThemedDialogWrapper(originalDialogProc, windowThemeManager);
}

Win32ResourceLoader::ThemedDialogWrapper::ThemedDialogWrapper(DialogProc originalDialogProc,
	WindowThemeManager *windowThemeManager) :
	m_originalDialogProc(originalDialogProc),
	m_windowThemeManager(windowThemeManager)
{
}

INT_PTR CALLBACK Win32ResourceLoader::ThemedDialogWrapper::ThemedDialogProcStub(HWND dialog,
	UINT msg, WPARAM wParam, LPARAM lParam)
{
	auto *themedDialogWrapper =
		reinterpret_cast<ThemedDialogWrapper *>(GetWindowLongPtr(dialog, GWLP_USERDATA));

	switch (msg)
	{
	case WM_INITDIALOG:
	{
		themedDialogWrapper = reinterpret_cast<ThemedDialogWrapper *>(lParam);

		SetLastError(0);
		auto res = SetWindowLongPtr(dialog, GWLP_USERDATA,
			reinterpret_cast<LONG_PTR>(themedDialogWrapper));
		CHECK(!(res == 0 && GetLastError() != 0));
	}
	break;

	case WM_NCDESTROY:
		SetWindowLongPtr(dialog, GWLP_USERDATA, 0);
		break;
	}

	if (!themedDialogWrapper)
	{
		return 0;
	}

	return themedDialogWrapper->ThemedDialogProc(dialog, msg, wParam, lParam);
}

INT_PTR Win32ResourceLoader::ThemedDialogWrapper::ThemedDialogProc(HWND dialog, UINT msg,
	WPARAM wParam, LPARAM lParam)
{
	auto res = m_originalDialogProc(dialog, msg, wParam, lParam);

	switch (msg)
	{
	case WM_INITDIALOG:
		// This is explicitly done after the original dialog procedure has been invoked, so that any
		// controls the dialog dynamically creates will be themed.
		m_themeWindowTracker = std::make_unique<ThemeWindowTracker>(dialog, m_windowThemeManager);
		break;

	case WM_NCDESTROY:
		delete this;
		break;
	}

	return res;
}
