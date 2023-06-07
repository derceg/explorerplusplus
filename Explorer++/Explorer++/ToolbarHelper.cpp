// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ToolbarHelper.h"
#include "Icon.h"
#include "IconResourceLoader.h"
#include "../Helper/Controls.h"
#include "../Helper/DpiCompatibility.h"

namespace ToolbarHelper
{

std::tuple<HWND, wil::unique_himagelist> CreateCloseButtonToolbar(HWND parent, int closeButtonId,
	const std::wstring &tooltip, IconResourceLoader *iconResourceLoader)
{
	HWND toolbar = CreateToolbar(parent,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS | TBSTYLE_LIST
			| TBSTYLE_TRANSPARENT | TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE,
		TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DOUBLEBUFFER);
	SendMessage(toolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(toolbar);
	wil::unique_hbitmap bitmap =
		iconResourceLoader->LoadBitmapFromPNGForDpi(Icon::CloseButton, 16, 16, dpi);

	BITMAP bitmapInfo;
	[[maybe_unused]] int res = GetObject(bitmap.get(), sizeof(bitmapInfo), &bitmapInfo);
	assert(res != 0);

	wil::unique_himagelist imageList(
		ImageList_Create(bitmapInfo.bmWidth, bitmapInfo.bmHeight, ILC_COLOR32 | ILC_MASK, 0, 1));
	int imageIndex = ImageList_Add(imageList.get(), bitmap.get(), nullptr);

	SendMessage(toolbar, TB_SETBITMAPSIZE, 0, MAKELONG(bitmapInfo.bmWidth, bitmapInfo.bmHeight));
	SendMessage(toolbar, TB_SETBUTTONSIZE, 0, MAKELPARAM(bitmapInfo.bmWidth, bitmapInfo.bmHeight));

	SendMessage(toolbar, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(imageList.get()));

	TBBUTTON tbButton = {};
	tbButton.iBitmap = imageIndex;
	tbButton.idCommand = closeButtonId;
	tbButton.fsState = TBSTATE_ENABLED;
	tbButton.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;
	tbButton.dwData = 0;
	tbButton.iString = reinterpret_cast<INT_PTR>(tooltip.c_str());
	SendMessage(toolbar, TB_INSERTBUTTON, 0, reinterpret_cast<LPARAM>(&tbButton));

	SendMessage(toolbar, TB_AUTOSIZE, 0, 0);

	return { toolbar, std::move(imageList) };
}

}
