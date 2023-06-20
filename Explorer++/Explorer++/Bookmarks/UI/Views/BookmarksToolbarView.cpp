// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarksToolbarView.h"

BookmarksToolbarView::BookmarksToolbarView(HWND parent, const Config *config) :
	ToolbarView(parent,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TBSTYLE_TOOLTIPS | TBSTYLE_LIST
			| TBSTYLE_TRANSPARENT | TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NORESIZE,
		TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_HIDECLIPPEDBUTTONS, config)
{
}

void BookmarksToolbarView::SetImageList(HIMAGELIST imageList)
{
	int iconWidth;
	int iconHeight;
	[[maybe_unused]] auto iconSizeRes = ImageList_GetIconSize(imageList, &iconWidth, &iconHeight);
	assert(iconSizeRes);

	[[maybe_unused]] auto res =
		SendMessage(m_hwnd, TB_SETBITMAPSIZE, 0, MAKELONG(iconWidth, iconHeight));
	assert(res);

	res = SendMessage(m_hwnd, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(imageList));
	assert(!res);
}
