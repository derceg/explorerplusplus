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
#include "../Helper/ResourceHelper.h"

Win32ResourceLoader::Win32ResourceLoader(HINSTANCE resourceInstance, IconSet iconSet,
	const DarkModeManager *darkModeManager, ThemeManager *themeManager) :
	m_resourceInstance(resourceInstance),
	m_iconSet(iconSet),
	m_darkModeManager(darkModeManager),
	m_themeManager(themeManager)
{
}

std::wstring Win32ResourceLoader::LoadString(UINT stringId) const
{
	// TODO: The implementation for ResourceHelper::LoadString should be moved into this method,
	// once all calls to it have been migrated to this method.
	return ResourceHelper::LoadString(m_resourceInstance, stringId);
}

std::optional<std::wstring> Win32ResourceLoader::MaybeLoadString(UINT stringId) const
{
	return ResourceHelper::MaybeLoadString(m_resourceInstance, stringId);
}

wil::unique_hbitmap Win32ResourceLoader::LoadBitmapFromPNGForDpi(Icon icon, int iconWidth,
	int iconHeight, int dpi) const
{
	auto gdiplusBitmap = LoadGdiplusBitmapFromPNGForDpi(icon, iconWidth, iconHeight, dpi);
	return ImageHelper::GdiplusBitmapToBitmap(gdiplusBitmap.get());
}

wil::unique_hbitmap Win32ResourceLoader::LoadBitmapFromPNGAndScale(Icon icon, int iconWidth,
	int iconHeight) const
{
	auto gdiplusBitmap = LoadGdiplusBitmapFromPNGAndScalePlusInvert(icon, iconWidth, iconHeight);
	return ImageHelper::GdiplusBitmapToBitmap(gdiplusBitmap.get());
}

wil::unique_hicon Win32ResourceLoader::LoadIconFromPNGForDpi(Icon icon, int iconWidth,
	int iconHeight, int dpi) const
{
	auto gdiplusBitmap = LoadGdiplusBitmapFromPNGForDpi(icon, iconWidth, iconHeight, dpi);
	return ImageHelper::GdiplusBitmapToIcon(gdiplusBitmap.get());
}

wil::unique_hicon Win32ResourceLoader::LoadIconFromPNGAndScale(Icon icon, int iconWidth,
	int iconHeight) const
{
	auto gdiplusBitmap = LoadGdiplusBitmapFromPNGAndScalePlusInvert(icon, iconWidth, iconHeight);
	return ImageHelper::GdiplusBitmapToIcon(gdiplusBitmap.get());
}

std::unique_ptr<Gdiplus::Bitmap> Win32ResourceLoader::LoadGdiplusBitmapFromPNGForDpi(Icon icon,
	int iconWidth, int iconHeight, int dpi) const
{
	int scaledIconWidth = MulDiv(iconWidth, dpi, USER_DEFAULT_SCREEN_DPI);
	int scaledIconHeight = MulDiv(iconHeight, dpi, USER_DEFAULT_SCREEN_DPI);
	return LoadGdiplusBitmapFromPNGAndScalePlusInvert(icon, scaledIconWidth, scaledIconHeight);
}

// Loads and scales a PNG and then inverts the colors, when required by the current color mode.
std::unique_ptr<Gdiplus::Bitmap> Win32ResourceLoader::LoadGdiplusBitmapFromPNGAndScalePlusInvert(
	Icon icon, int iconWidth, int iconHeight) const
{
	auto bitmap = LoadGdiplusBitmapFromPNGAndScale(icon, iconWidth, iconHeight);

	if (m_iconSet == +IconSet::Color || !m_darkModeManager->IsDarkModeEnabled())
	{
		return bitmap;
	}

	auto invertedBitmap = std::make_unique<Gdiplus::Bitmap>(iconWidth, iconHeight);
	invertedBitmap->SetResolution(bitmap->GetHorizontalResolution(),
		bitmap->GetVerticalResolution());

	Gdiplus::Graphics graphics(invertedBitmap.get());

	// This matrix will result in the RGB components all being inverted, while the alpha component
	// will stay as-is. See the documentation on ColorMatrix for information on how this structure
	// is laid out.
	// clang-format off
	Gdiplus::ColorMatrix colorMatrix = {
		-1, 0, 0, 0, 0,
		0, -1, 0, 0, 0,
		0, 0, -1, 0, 0,
		0, 0, 0, 1, 0,
		1, 1, 1, 0, 1
	};
	// clang-format on

	Gdiplus::ImageAttributes attributes;
	attributes.SetColorMatrix(&colorMatrix);

	graphics.DrawImage(bitmap.get(), Gdiplus::Rect(0, 0, iconWidth, iconHeight), 0, 0, iconWidth,
		iconHeight, Gdiplus::UnitPixel, &attributes);

	return invertedBitmap;
}

// This function is based on the steps performed by
// https://docs.microsoft.com/en-us/windows/win32/api/commctrl/nf-commctrl-loadiconmetric when
// loading an icon (see the remarks section on that page for details).
std::unique_ptr<Gdiplus::Bitmap> Win32ResourceLoader::LoadGdiplusBitmapFromPNGAndScale(Icon icon,
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

	const auto &iconSizeMappins = mapping->at(icon);

	auto match = std::find_if(iconSizeMappins.begin(), iconSizeMappins.end(),
		[iconWidth, iconHeight](auto entry)
		{ return iconWidth <= entry.first && iconHeight <= entry.first; });

	if (match == iconSizeMappins.end())
	{
		match = std::prev(iconSizeMappins.end());
	}

	// Images are always loaded from the executable's resource instance.
	auto bitmap = ImageHelper::LoadGdiplusBitmapFromPNG(GetModuleHandle(nullptr), match->second);
	DCHECK(bitmap);

	// If the icon size matches exactly, it doesn't need to be scaled, so can be
	// returned immediately.
	if (match->first == iconWidth && match->first == iconHeight)
	{
		return bitmap;
	}

	auto scaledBitmap = std::make_unique<Gdiplus::Bitmap>(iconWidth, iconHeight);
	scaledBitmap->SetResolution(bitmap->GetHorizontalResolution(), bitmap->GetVerticalResolution());

	Gdiplus::Graphics graphics(scaledBitmap.get());

	float scalingFactorX = static_cast<float>(iconWidth) / static_cast<float>(match->first);
	float scalingFactorY = static_cast<float>(iconHeight) / static_cast<float>(match->first);
	graphics.ScaleTransform(scalingFactorX, scalingFactorY);
	graphics.DrawImage(bitmap.get(), 0, 0);

	return scaledBitmap;
}

INT_PTR Win32ResourceLoader::CreateModalDialog(UINT dialogId, HWND parent,
	DialogProc dialogProc) const
{
	INT_PTR res;

	auto *themedDialogWrapper = ThemedDialogWrapper::Create(dialogProc, m_themeManager);

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

	auto *themedDialogWrapper = ThemedDialogWrapper::Create(dialogProc, m_themeManager);

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
	ThemedDialogWrapper::Create(DialogProc originalDialogProc, ThemeManager *themeManager)
{
	return new ThemedDialogWrapper(originalDialogProc, themeManager);
}

Win32ResourceLoader::ThemedDialogWrapper::ThemedDialogWrapper(DialogProc originalDialogProc,
	ThemeManager *themeManager) :
	m_originalDialogProc(originalDialogProc),
	m_themeManager(themeManager)
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
		m_themeWindowTracker = std::make_unique<ThemeWindowTracker>(dialog, m_themeManager);
		break;

	case WM_NCDESTROY:
		delete this;
		break;
	}

	return res;
}
