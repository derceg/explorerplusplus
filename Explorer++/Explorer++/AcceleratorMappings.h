// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MainResource.h"
#include <unordered_map>

const std::unordered_map<std::wstring, int> ACCELERATOR_MAPPINGS = {
	// Accelerators.
	{ L"address_bar", IDA_ADDRESSBAR },
	{ L"address_bar_dropdown", IDA_COMBODROPDOWN },
	{ L"home", IDA_HOME },
	{ L"select_last_tab", IDA_SELECT_LAST_TAB },
	{ L"select_next_tab", IDA_SELECT_NEXT_TAB },
	{ L"focus_next_window", IDA_NEXTWINDOW },
	{ L"select_previous_tab", IDA_SELECT_PREVIOUS_TAB },
	{ L"focus_previous_window", IDA_PREVIOUSWINDOW },
	{ L"select_tab_1", IDA_SELECT_TAB_1 },
	{ L"select_tab_2", IDA_SELECT_TAB_2 },
	{ L"select_tab_3", IDA_SELECT_TAB_3 },
	{ L"select_tab_4", IDA_SELECT_TAB_4 },
	{ L"select_tab_5", IDA_SELECT_TAB_5 },
	{ L"select_tab_6", IDA_SELECT_TAB_6 },
	{ L"select_tab_7", IDA_SELECT_TAB_7 },
	{ L"select_tab_8", IDA_SELECT_TAB_8 },
	{ L"duplicate_tab", IDA_DUPLICATE_TAB },

	// Main menu items.
	{ L"new_tab", IDM_FILE_NEWTAB },
	{ L"close_tab", IDM_FILE_CLOSETAB },
	{ L"clone_window", IDM_FILE_CLONEWINDOW },
	{ L"save_directory_listing", IDM_FILE_SAVEDIRECTORYLISTING },
	{ L"open_command_prompt", IDM_FILE_OPENCOMMANDPROMPT },
	{ L"open_command_prompt_as_administrator", IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR },
	{ L"copy_folder_path", IDM_FILE_COPYFOLDERPATH },
	{ L"copy_file_paths", IDM_FILE_COPYITEMPATH },
	{ L"copy_universal_file_paths", IDM_FILE_COPYUNIVERSALFILEPATHS },
	{ L"copy_column_text", IDM_FILE_COPYCOLUMNTEXT },
	{ L"set_file_attributes", IDM_FILE_SETFILEATTRIBUTES },
	{ L"delete_permanently", IDM_FILE_DELETEPERMANENTLY },
	{ L"rename", IDM_FILE_RENAME },
	{ L"properties", IDM_FILE_PROPERTIES },
	{ L"exit", IDM_FILE_EXIT },

	{ L"undo", IDM_EDIT_UNDO },
	{ L"paste_shortcut", IDM_EDIT_PASTESHORTCUT },
	{ L"paste_hard_link", IDM_EDIT_PASTEHARDLINK },
	{ L"copy_to_folder", IDM_EDIT_COPYTOFOLDER },
	{ L"move_to_folder", IDM_EDIT_MOVETOFOLDER },
	{ L"resolve_link", IDM_EDIT_RESOLVELINK },

	{ L"select_all_of_same_type", IDM_EDIT_SELECTALLOFSAMETYPE },
	{ L"select_none", IDM_EDIT_SELECTNONE },
	{ L"wildcard_deselect", IDM_EDIT_WILDCARDDESELECT },
	{ L"wildcard_select", IDM_EDIT_WILDCARDSELECTION },

	{ L"toggle_status_bar", IDM_VIEW_STATUSBAR },
	{ L"toggle_folders", IDM_VIEW_FOLDERS },
	{ L"toggle_display_window", IDM_VIEW_DISPLAYWINDOW },

	{ L"auto_arrange", IDM_VIEW_AUTOARRANGE },
	{ L"toggle_hidden_files", IDM_VIEW_SHOWHIDDENFILES },
	{ L"refresh", IDM_VIEW_REFRESH },

	{ L"select_columns", IDM_VIEW_SELECTCOLUMNS },
	{ L"autosize_columns", IDM_VIEW_AUTOSIZECOLUMNS },
	{ L"save_column_layout_as_default", IDM_VIEW_SAVECOLUMNLAYOUTASDEFAULT },
	{ L"change_display_window_colors", IDM_VIEW_CHANGEDISPLAYCOLOURS },

	{ L"set_filter", IDM_FILTER_FILTERRESULTS },
	{ L"enable_filter", IDM_FILTER_ENABLE_FILTER },

	{ L"new_folder", IDM_ACTIONS_NEWFOLDER },
	{ L"split_file", IDM_ACTIONS_SPLITFILE },
	{ L"merge_files", IDM_ACTIONS_MERGEFILES },
	{ L"destroy_files", IDM_ACTIONS_DESTROYFILES },

	{ L"back", IDM_GO_BACK },
	{ L"forward", IDM_GO_FORWARD },
	{ L"up", IDM_GO_UP },
	{ L"go_quick_access", IDM_GO_QUICK_ACCESS },
	{ L"go_computer", IDM_GO_COMPUTER },
	{ L"go_documents", IDM_GO_DOCUMENTS },
	{ L"go_downloads", IDM_GO_DOWNLOADS },
	{ L"go_music", IDM_GO_MUSIC },
	{ L"go_pictures", IDM_GO_PICTURES },
	{ L"go_videos", IDM_GO_VIDEOS },
	{ L"go_desktop", IDM_GO_DESKTOP },
	{ L"go_recycle_bin", IDM_GO_RECYCLE_BIN },
	{ L"go_control_panel", IDM_GO_CONTROL_PANEL },
	{ L"go_printers", IDM_GO_PRINTERS },
	{ L"go_network_places", IDM_GO_NETWORK },
	{ L"go_wsl_distributions", IDM_GO_WSL_DISTRIBUTIONS },

	{ L"bookmark_tab", IDM_BOOKMARKS_BOOKMARK_THIS_TAB },
	{ L"manage_bookmarks", IDM_BOOKMARKS_MANAGE_BOOKMARKS },

	{ L"search", IDM_TOOLS_SEARCH },
	{ L"customize_colors", IDM_TOOLS_CUSTOMIZECOLORS },
	{ L"run_script", IDM_TOOLS_RUNSCRIPT },
	{ L"options", IDM_TOOLS_OPTIONS },

	{ L"help", IDM_HELP_ONLINE_DOCUMENTATION },
	{ L"check_for_updates", IDM_HELP_CHECKFORUPDATES },
	{ L"about", IDM_HELP_ABOUT },
};
