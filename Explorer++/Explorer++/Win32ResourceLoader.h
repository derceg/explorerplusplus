// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "IconSet.h"
#include "ResourceLoader.h"
#include <gdiplus.h>
#include <memory>

class DarkModeManager;
class ThemeManager;
class ThemeWindowTracker;

// Loads a resource, using the Windows API resource functions.
class Win32ResourceLoader : public ResourceLoader
{
public:
	Win32ResourceLoader(HINSTANCE resourceInstance, IconSet iconSet,
		const DarkModeManager *darkModeManager, ThemeManager *themeManager);

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

	INT_PTR CreateModalDialog(UINT dialogId, HWND parent, DialogProc dialogProc) const override;
	HWND CreateModelessDialog(UINT dialogId, HWND parent, DialogProc dialogProc) const override;

private:
	// Wraps a dialog procedure. This class has two advantages:
	//
	// 1. It allows the dialog to be themed on creation.
	// 2. It means the client can pass in a dialog procedure using a std::function, rather than via
	//    a raw function pointer.
	class ThemedDialogWrapper
	{
	public:
		static ThemedDialogWrapper *Create(DialogProc originalDialogProc,
			ThemeManager *themeManager);

		static INT_PTR CALLBACK ThemedDialogProcStub(HWND dialog, UINT msg, WPARAM wParam,
			LPARAM lParam);

	private:
		ThemedDialogWrapper(DialogProc originalDialogProc, ThemeManager *themeManager);

		INT_PTR ThemedDialogProc(HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam);

		const DialogProc m_originalDialogProc;
		ThemeManager *const m_themeManager;
		std::unique_ptr<ThemeWindowTracker> m_themeWindowTracker;
	};

	std::unique_ptr<Gdiplus::Bitmap> LoadGdiplusBitmapFromPNGForDpi(Icon icon, int iconWidth,
		int iconHeight, int dpi) const;
	std::unique_ptr<Gdiplus::Bitmap> LoadGdiplusBitmapFromPNGAndScalePlusInvert(Icon icon,
		int iconWidth, int iconHeight) const;
	std::unique_ptr<Gdiplus::Bitmap> LoadGdiplusBitmapFromPNGAndScale(Icon icon, int iconWidth,
		int iconHeight) const;

	const HINSTANCE m_resourceInstance;
	const IconSet m_iconSet;
	const DarkModeManager *const m_darkModeManager;
	ThemeManager *const m_themeManager;
};
