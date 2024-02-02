// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarksToolbarView.h"
#include <glog/logging.h>

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
	auto iconSizeRes = ImageList_GetIconSize(imageList, &iconWidth, &iconHeight);
	DCHECK(iconSizeRes);

	auto res = SendMessage(m_hwnd, TB_SETBITMAPSIZE, 0, MAKELONG(iconWidth, iconHeight));
	DCHECK(res);

	SendMessage(m_hwnd, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(imageList));
}
