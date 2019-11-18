// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Explorer++_internal.h"
#include "ToolbarButtons.h"

struct ToolbarButton_t
{
	int iItemID;
};

const ToolbarButton_t DEFAULT_TOOLBAR_BUTTONS[] =
{
	{TOOLBAR_BACK},
	{TOOLBAR_FORWARD},
	{TOOLBAR_UP},
	{TOOLBAR_SEPARATOR},
	{TOOLBAR_FOLDERS},
	{TOOLBAR_SEPARATOR},
	{TOOLBAR_CUT},
	{TOOLBAR_COPY},
	{TOOLBAR_PASTE},
	{TOOLBAR_DELETE},
	{TOOLBAR_DELETE_PERMANENTLY},
	{TOOLBAR_PROPERTIES},
	{TOOLBAR_SEARCH},
	{TOOLBAR_SEPARATOR},
	{TOOLBAR_NEW_FOLDER},
	{TOOLBAR_COPY_TO},
	{TOOLBAR_MOVE_TO},
	{TOOLBAR_SEPARATOR},
	{TOOLBAR_VIEWS},
	{TOOLBAR_OPEN_COMMAND_PROMPT},
	{TOOLBAR_REFRESH},
	{TOOLBAR_SEPARATOR},
	{TOOLBAR_ADD_BOOKMARK},
	{TOOLBAR_ORGANIZE_BOOKMARKS}
};