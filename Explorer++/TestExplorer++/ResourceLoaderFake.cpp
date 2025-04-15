// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ResourceLoaderFake.h"

std::wstring ResourceLoaderFake::LoadString(UINT stringId) const
{
	UNREFERENCED_PARAMETER(stringId);

	return L"";
}

std::optional<std::wstring> ResourceLoaderFake::MaybeLoadString(UINT stringId) const
{
	UNREFERENCED_PARAMETER(stringId);

	return L"";
}

wil::unique_hbitmap ResourceLoaderFake::LoadBitmapFromPNGForDpi(Icon icon, int iconWidth,
	int iconHeight, int dpi) const
{
	UNREFERENCED_PARAMETER(icon);
	UNREFERENCED_PARAMETER(iconWidth);
	UNREFERENCED_PARAMETER(iconHeight);
	UNREFERENCED_PARAMETER(dpi);

	return nullptr;
}

wil::unique_hbitmap ResourceLoaderFake::LoadBitmapFromPNGAndScale(Icon icon, int iconWidth,
	int iconHeight) const
{
	UNREFERENCED_PARAMETER(icon);
	UNREFERENCED_PARAMETER(iconWidth);
	UNREFERENCED_PARAMETER(iconHeight);

	return nullptr;
}

wil::unique_hicon ResourceLoaderFake::LoadIconFromPNGForDpi(Icon icon, int iconWidth,
	int iconHeight, int dpi) const
{
	UNREFERENCED_PARAMETER(icon);
	UNREFERENCED_PARAMETER(iconWidth);
	UNREFERENCED_PARAMETER(iconHeight);
	UNREFERENCED_PARAMETER(dpi);

	return nullptr;
}

wil::unique_hicon ResourceLoaderFake::LoadIconFromPNGAndScale(Icon icon, int iconWidth,
	int iconHeight) const
{
	UNREFERENCED_PARAMETER(icon);
	UNREFERENCED_PARAMETER(iconWidth);
	UNREFERENCED_PARAMETER(iconHeight);

	return nullptr;
}

INT_PTR ResourceLoaderFake::CreateModalDialog(UINT dialogId, HWND parent, DLGPROC dialogProc,
	LPARAM initParam) const
{
	UNREFERENCED_PARAMETER(dialogId);
	UNREFERENCED_PARAMETER(parent);
	UNREFERENCED_PARAMETER(dialogProc);
	UNREFERENCED_PARAMETER(initParam);

	return 0;
}

HWND ResourceLoaderFake::CreateModelessDialog(UINT dialogId, HWND parent, DLGPROC dialogProc,
	LPARAM initParam) const
{
	UNREFERENCED_PARAMETER(dialogId);
	UNREFERENCED_PARAMETER(parent);
	UNREFERENCED_PARAMETER(dialogProc);
	UNREFERENCED_PARAMETER(initParam);

	return nullptr;
}
