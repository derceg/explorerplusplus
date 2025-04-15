// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "IconSet.h"
#include "ResourceLoader.h"
#include <gdiplus.h>

class DarkModeManager;

// Loads a resource, using the Windows API resource functions.
class Win32ResourceLoader : public ResourceLoader
{
public:
	Win32ResourceLoader(HINSTANCE resourceInstance, IconSet iconSet,
		const DarkModeManager *darkModeManager);

	std::wstring LoadString(UINT stringId) const override;
	std::optional<std::wstring> MaybeLoadString(UINT stringId) const override;

	wil::unique_hbitmap LoadBitmapFromPNGForDpi(Icon icon, int iconWidth, int iconHeight,
		int dpi) const override;
	wil::unique_hbitmap LoadBitmapFromPNGAndScale(Icon icon, int iconWidth,
		int iconHeight) const override;
	wil::unique_hicon LoadIconFromPNGForDpi(Icon icon, int iconWidth, int iconHeight,
		int dpi) const override;
	wil::unique_hicon LoadIconFromPNGAndScale(Icon icon, int iconWidth,
		int iconHeight) const override;

	INT_PTR CreateModalDialog(UINT dialogId, HWND parent, DLGPROC dialogProc,
		LPARAM initParam) const override;
	HWND CreateModelessDialog(UINT dialogId, HWND parent, DLGPROC dialogProc,
		LPARAM initParam) const override;

private:
	std::unique_ptr<Gdiplus::Bitmap> LoadGdiplusBitmapFromPNGForDpi(Icon icon, int iconWidth,
		int iconHeight, int dpi) const;
	std::unique_ptr<Gdiplus::Bitmap> LoadGdiplusBitmapFromPNGAndScalePlusInvert(Icon icon,
		int iconWidth, int iconHeight) const;
	std::unique_ptr<Gdiplus::Bitmap> LoadGdiplusBitmapFromPNGAndScale(Icon icon, int iconWidth,
		int iconHeight) const;

	const HINSTANCE m_resourceInstance;
	const IconSet m_iconSet;
	const DarkModeManager *const m_darkModeManager;
};
