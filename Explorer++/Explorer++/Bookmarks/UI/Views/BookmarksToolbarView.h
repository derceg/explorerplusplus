// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ToolbarView.h"

class BookmarksToolbarView : public ToolbarView
{
public:
	BookmarksToolbarView(HWND parent, const Config *config);

	void SetImageList(HIMAGELIST imageList);
};
