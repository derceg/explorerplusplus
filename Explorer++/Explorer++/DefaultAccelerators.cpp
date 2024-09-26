// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DefaultAccelerators.h"
#include "AcceleratorHelper.h"
#include "AcceleratorManager.h"
#include "MainResource.h"

namespace
{

// Note that the ordering of the items below has some importance. For items that have multiple
// accelerator items defined, the first accelerator that appears will be used as the "default"
// accelerator (i.e. the accelerator used in any associated menu items).
// clang-format off
constexpr ACCEL g_defaultAccelerators[] = {
	{ FVIRTKEY | FALT, 'D', IDA_ADDRESSBAR },
	{ FVIRTKEY | FCONTROL, 'L', IDA_ADDRESSBAR },
	{ FVIRTKEY, VK_F4, IDA_COMBODROPDOWN },
	{ FVIRTKEY | FALT, VK_HOME, IDA_HOME },
	{ FVIRTKEY | FCONTROL, '9', IDA_LASTTAB },
	{ FVIRTKEY | FCONTROL, VK_NEXT, IDA_NEXTTAB },
	{ FVIRTKEY | FCONTROL, VK_OEM_6, IDA_NEXTTAB },
	{ FVIRTKEY | FCONTROL, VK_TAB, IDA_NEXTTAB },
	{ FVIRTKEY, VK_F6, IDA_NEXTWINDOW },
	{ FVIRTKEY, VK_TAB, IDA_NEXTWINDOW },
	{ FVIRTKEY | FCONTROL, VK_OEM_4, IDA_PREVIOUSTAB },
	{ FVIRTKEY | FCONTROL, VK_PRIOR, IDA_PREVIOUSTAB },
	{ FVIRTKEY | FCONTROL | FSHIFT, VK_TAB, IDA_PREVIOUSTAB },
	{ FVIRTKEY | FSHIFT, VK_F6, IDA_PREVIOUSWINDOW },
	{ FVIRTKEY | FSHIFT, VK_TAB, IDA_PREVIOUSWINDOW },
	{ FVIRTKEY | FCONTROL, '0', IDA_RESET_TEXT_SIZE },
	{ FVIRTKEY | FCONTROL, VK_NUMPAD0, IDA_RESET_TEXT_SIZE },
	{ FVIRTKEY | FCONTROL | FSHIFT, 'T', IDA_RESTORE_LAST_TAB },
	{ FVIRTKEY | FCONTROL, '1', IDA_TAB1 },
	{ FVIRTKEY | FCONTROL, '2', IDA_TAB2 },
	{ FVIRTKEY | FCONTROL, '3', IDA_TAB3 },
	{ FVIRTKEY | FCONTROL, '4', IDA_TAB4 },
	{ FVIRTKEY | FCONTROL, '5', IDA_TAB5 },
	{ FVIRTKEY | FCONTROL, '6', IDA_TAB6 },
	{ FVIRTKEY | FCONTROL, '7', IDA_TAB7 },
	{ FVIRTKEY | FCONTROL, '8', IDA_TAB8 },
	{ FVIRTKEY | FCONTROL, 'E', IDA_TAB_DUPLICATETAB },
	{ FVIRTKEY | FCONTROL, 'N', IDM_ACTIONS_NEWFOLDER },
	{ FVIRTKEY | FCONTROL | FSHIFT, 'D', IDM_BOOKMARKS_BOOKMARK_ALL_TABS },
	{ FVIRTKEY | FCONTROL, 'D', IDM_BOOKMARKS_BOOKMARKTHISTAB },
	{ FVIRTKEY | FCONTROL, 'B', IDM_BOOKMARKS_MANAGEBOOKMARKS },
	{ FVIRTKEY | FCONTROL | FSHIFT, 'C', IDM_EDIT_COPYTOFOLDER },
	{ FVIRTKEY | FCONTROL | FSHIFT, 'M', IDM_EDIT_MOVETOFOLDER },
	{ FVIRTKEY | FCONTROL | FSHIFT, 'V', IDM_EDIT_PASTESHORTCUT },
	{ FVIRTKEY | FCONTROL | FSHIFT, 'N', IDM_EDIT_SELECTNONE },
	{ FVIRTKEY | FCONTROL | FSHIFT, 'W', IDM_EDIT_WILDCARDDESELECT },
	{ FVIRTKEY | FCONTROL | FSHIFT, 'S', IDM_EDIT_WILDCARDSELECTION },
	{ FVIRTKEY | FCONTROL, 'W', IDM_FILE_CLOSETAB },
	{ FVIRTKEY | FCONTROL, 'Q', IDM_FILE_EXIT },
	{ FVIRTKEY | FCONTROL, VK_F4, IDM_FILE_CLOSETAB },
	{ FVIRTKEY | FCONTROL | FSHIFT, 'P', IDM_FILE_COPYFOLDERPATH },
	{ FVIRTKEY | FSHIFT, VK_DELETE, IDM_FILE_DELETEPERMANENTLY },
	{ FVIRTKEY | FCONTROL, 'T', IDM_FILE_NEWTAB },
	{ FVIRTKEY | FALT, VK_RETURN, IDM_FILE_PROPERTIES },
	{ FVIRTKEY, VK_F2, IDM_FILE_RENAME },
	{ FVIRTKEY | FCONTROL, 'G', IDM_FILTER_APPLYFILTER },
	{ FVIRTKEY | FCONTROL | FSHIFT, 'F', IDM_FILTER_FILTERRESULTS },
	{ FVIRTKEY | FALT, VK_LEFT, IDM_GO_BACK },
	{ FVIRTKEY | FALT, VK_RIGHT, IDM_GO_FORWARD },
	{ FVIRTKEY | FALT, VK_UP, IDM_GO_UP },
	{ FVIRTKEY, VK_F1, IDM_HELP_ONLINE_DOCUMENTATION },
	{ FVIRTKEY | FCONTROL, 'F', IDM_TOOLS_SEARCH },
	{ FVIRTKEY, VK_F3, IDM_TOOLS_SEARCH },
	{ FVIRTKEY | FCONTROL, VK_SUBTRACT, IDM_VIEW_DECREASE_TEXT_SIZE },
	{ FVIRTKEY | FCONTROL, VK_OEM_MINUS, IDM_VIEW_DECREASE_TEXT_SIZE },
	{ FVIRTKEY | FCONTROL | FSHIFT, '5', IDM_VIEW_DETAILS },
	{ FVIRTKEY | FCONTROL | FSHIFT, '0', IDM_VIEW_EXTRALARGEICONS },
	{ FVIRTKEY | FCONTROL | FSHIFT, '2', IDM_VIEW_ICONS },
	{ FVIRTKEY | FCONTROL, VK_ADD, IDM_VIEW_INCREASE_TEXT_SIZE },
	{ FVIRTKEY | FCONTROL, VK_OEM_PLUS, IDM_VIEW_INCREASE_TEXT_SIZE },
	{ FVIRTKEY | FCONTROL | FSHIFT, '1', IDM_VIEW_LARGEICONS },
	{ FVIRTKEY | FCONTROL | FSHIFT, '4', IDM_VIEW_LIST },
	{ FVIRTKEY, VK_F5, IDM_VIEW_REFRESH },
	{ FVIRTKEY | FCONTROL, 'R', IDM_VIEW_REFRESH },
	{ FVIRTKEY | FCONTROL, 'H', IDM_VIEW_SHOWHIDDENFILES },
	{ FVIRTKEY | FCONTROL | FSHIFT, '3', IDM_VIEW_SMALLICONS },
	{ FVIRTKEY | FCONTROL | FSHIFT, '8', IDM_VIEW_THUMBNAILS },
	{ FVIRTKEY | FCONTROL | FSHIFT, '9', IDM_VIEW_TILES },
	{ FVIRTKEY | FCONTROL | FSHIFT, 'A', IDM_WINDOW_SEARCH_TABS },
	{ FVIRTKEY | FCONTROL | FSHIFT, '6', IDM_VIEW_EXTRALARGETHUMBNAILS },
	{ FVIRTKEY | FCONTROL | FSHIFT, '7', IDM_VIEW_LARGETHUMBNAILS }
};
// clang-format on

static_assert(AreAcceleratorsValid(g_defaultAccelerators));

// Some shortcut combinations are scoped to particular controls. Despite that fact, the shortcuts
// should still be shown in menu entries. That's the reason for the array here.
// Note that this is considered temporary. These shortcuts should be handled jointly with the rest
// of the accelerator items. When that's done, this array can be removed.
// clang-format off
constexpr ACCEL g_nonAcceleratorShortcuts[] = {
	{ FVIRTKEY | FCONTROL, 'C', IDM_EDIT_COPY },
	{ FVIRTKEY | FCONTROL, 'X', IDM_EDIT_CUT },
	{ FVIRTKEY | FCONTROL, 'I', IDM_EDIT_INVERTSELECTION },
	{ FVIRTKEY | FCONTROL, 'V', IDM_EDIT_PASTE },
	{ FVIRTKEY | FCONTROL, 'A', IDM_EDIT_SELECTALL },
	{ FVIRTKEY | FCONTROL, 'Z', IDM_EDIT_UNDO },
	{ FVIRTKEY, VK_DELETE, IDM_FILE_DELETE }
};
// clang-format on

}

AcceleratorManager InitializeAcceleratorManager()
{
	return { g_defaultAccelerators, g_nonAcceleratorShortcuts };
}
