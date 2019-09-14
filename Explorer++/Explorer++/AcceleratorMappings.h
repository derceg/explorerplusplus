// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MainResource.h"
#include <unordered_map>

const std::unordered_map<std::wstring, int> ACCELERATOR_MAPPINGS = {
	{L"address_bar", IDA_ADDRESSBAR},
	{L"address_bar_dropdown", IDA_COMBODROPDOWN},
	{L"rename", IDA_FILE_RENAME},
	{L"help", IDA_HELP_HELP},
	{L"home", IDA_HOME},
	{L"select_last_tab", IDA_LASTTAB},
	{L"select_next_tab", IDA_NEXTTAB},
	{L"focus_next_window", IDA_NEXTWINDOW},
	{L"select_previous_tab", IDA_PREVIOUSTAB},
	{L"focus_previous_window", IDA_PREVIOUSWINDOW},
	{L"select_tab_1", IDA_TAB1},
	{L"select_tab_2", IDA_TAB2},
	{L"select_tab_3", IDA_TAB3},
	{L"select_tab_4", IDA_TAB4},
	{L"select_tab_5", IDA_TAB5},
	{L"select_tab_6", IDA_TAB6},
	{L"select_tab_7", IDA_TAB7},
	{L"select_tab_8", IDA_TAB8},
	{L"duplicate_tab", IDA_TAB_DUPLICATETAB},
	{L"new_folder", IDM_ACTIONS_NEWFOLDER},
	{L"bookmark_tab", IDM_BOOKMARKS_BOOKMARKTHISTAB},
	{L"manage_bookmarks", IDM_BOOKMARKS_MANAGEBOOKMARKS},
	{L"copy_to_folder", IDM_EDIT_COPYTOFOLDER},
	{L"move_to_folder", IDM_EDIT_MOVETOFOLDER},
	{L"paste_shortcut", IDM_EDIT_PASTESHORTCUT},
	{L"select_none", IDM_EDIT_SELECTNONE},
	{L"undo", IDM_EDIT_UNDO},
	{L"wildcard_deselect", IDM_EDIT_WILDCARDDESELECT},
	{L"wildcard_select", IDM_EDIT_WILDCARDSELECTION},
	{L"close_tab", IDM_FILE_CLOSETAB},
	{L"copy_folder_path", IDM_FILE_COPYFOLDERPATH},
	{L"delete_permanently", IDM_FILE_DELETEPERMANENTLY},
	{L"new_tab", IDM_FILE_NEWTAB},
	{L"properties", IDM_FILE_PROPERTIES},
	{L"apply_filter", IDM_FILTER_APPLYFILTER},
	{L"set_filter", IDM_FILTER_FILTERRESULTS},
	{L"back", IDM_GO_BACK},
	{L"forward", IDM_GO_FORWARD},
	{L"up", IDM_GO_UPONELEVEL},
	{L"search", IDM_TOOLS_SEARCH},
	{L"refresh", IDM_VIEW_REFRESH},
	{L"toggle_hidden_files", IDM_VIEW_SHOWHIDDENFILES}

	// The accelerators below are specifically excluded from customization.
	// {L"right_click", IDA_RCLICK}
};