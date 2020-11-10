// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Icon.h"
#include "MainResource.h"
#include <map>
#include <unordered_map>

using IconMapping = std::unordered_map<Icon, std::map<int, UINT>>;

// clang-format off
#define ICON_SIZE_MAPPINGS(BaseResourceId) \
	{ 16, BaseResourceId##_16 }, \
	{ 24, BaseResourceId##_24 }, \
	{ 32, BaseResourceId##_32 }, \
	{ 48, BaseResourceId##_48 }
// clang-format on

// clang-format off
#define ICON_RESOURCE_MAPPINGS(SetSuffix) \
	{ Icon::AddBookmark, { ICON_SIZE_MAPPINGS(IDB_ADD_BOOKMARK##SetSuffix) } }, \
	{ Icon::ArrowRight, { ICON_SIZE_MAPPINGS(IDB_ARROW_RIGHT##SetSuffix) } }, \
	{ Icon::Back, { ICON_SIZE_MAPPINGS(IDB_BACK##SetSuffix) } }, \
	{ Icon::Bookmarks, { ICON_SIZE_MAPPINGS(IDB_BOOKMARKS##SetSuffix) } }, \
	{ Icon::CloseButton, { ICON_SIZE_MAPPINGS(IDB_CLOSE_BUTTON##SetSuffix) } }, \
	{ Icon::CloseTab, { ICON_SIZE_MAPPINGS(IDB_CLOSE_TAB##SetSuffix) } }, \
	{ Icon::CommandLine, { ICON_SIZE_MAPPINGS(IDB_COMMAND_LINE##SetSuffix) } }, \
	{ Icon::CommandLineAdmin, { ICON_SIZE_MAPPINGS(IDB_COMMAND_LINE_ADMIN##SetSuffix) } }, \
	{ Icon::Copy, { ICON_SIZE_MAPPINGS(IDB_COPY##SetSuffix) } }, \
	{ Icon::CopyTo, { ICON_SIZE_MAPPINGS(IDB_COPY_TO##SetSuffix) } }, \
	{ Icon::CustomizeColors, { ICON_SIZE_MAPPINGS(IDB_CUSTOMIZE_COLORS##SetSuffix) } }, \
	{ Icon::Cut, { ICON_SIZE_MAPPINGS(IDB_CUT##SetSuffix) } }, \
	{ Icon::Delete, { ICON_SIZE_MAPPINGS(IDB_DELETE##SetSuffix) } }, \
	{ Icon::DeletePermanently, { ICON_SIZE_MAPPINGS(IDB_DELETE_PERMANENTLY##SetSuffix) } }, \
	{ Icon::Filter, { ICON_SIZE_MAPPINGS(IDB_FILTER##SetSuffix) } }, \
	{ Icon::Folder, { ICON_SIZE_MAPPINGS(IDB_FOLDER##SetSuffix) } }, \
	{ Icon::FolderTree, { ICON_SIZE_MAPPINGS(IDB_FOLDER_TREE##SetSuffix) } }, \
	{ Icon::Forward, { ICON_SIZE_MAPPINGS(IDB_FORWARD##SetSuffix) } }, \
	{ Icon::Help, { ICON_SIZE_MAPPINGS(IDB_HELP##SetSuffix) } }, \
	{ Icon::Lock, { ICON_SIZE_MAPPINGS(IDB_LOCK##SetSuffix) } }, \
	{ Icon::MassRename, { ICON_SIZE_MAPPINGS(IDB_MASS_RENAME##SetSuffix) } }, \
	{ Icon::MergeFiles, { ICON_SIZE_MAPPINGS(IDB_MERGE_FILES##SetSuffix) } }, \
	{ Icon::MoveTo, { ICON_SIZE_MAPPINGS(IDB_MOVE_TO##SetSuffix) } }, \
	{ Icon::NewFolder, { ICON_SIZE_MAPPINGS(IDB_NEW_FOLDER##SetSuffix) } }, \
	{ Icon::NewTab, { ICON_SIZE_MAPPINGS(IDB_NEW_TAB##SetSuffix) } }, \
	{ Icon::Options, { ICON_SIZE_MAPPINGS(IDB_OPTIONS##SetSuffix) } }, \
	{ Icon::Paste, { ICON_SIZE_MAPPINGS(IDB_PASTE##SetSuffix) } }, \
	{ Icon::PasteShortcut, { ICON_SIZE_MAPPINGS(IDB_PASTE_SHORTCUT##SetSuffix) } }, \
	{ Icon::Properties, { ICON_SIZE_MAPPINGS(IDB_PROPERTIES##SetSuffix) } }, \
	{ Icon::Refresh, { ICON_SIZE_MAPPINGS(IDB_REFRESH##SetSuffix) } }, \
	{ Icon::Rename, { ICON_SIZE_MAPPINGS(IDB_RENAME##SetSuffix) } }, \
	{ Icon::Search, { ICON_SIZE_MAPPINGS(IDB_SEARCH##SetSuffix) } }, \
	{ Icon::SelectColumns, { ICON_SIZE_MAPPINGS(IDB_SELECT_COLUMNS##SetSuffix) } }, \
	{ Icon::SplitFiles, { ICON_SIZE_MAPPINGS(IDB_SPLIT_FILES##SetSuffix) } }, \
	{ Icon::Undo, { ICON_SIZE_MAPPINGS(IDB_UNDO##SetSuffix) } }, \
	{ Icon::Up, { ICON_SIZE_MAPPINGS(IDB_UP##SetSuffix) } }, \
	{ Icon::Views, { ICON_SIZE_MAPPINGS(IDB_VIEWS##SetSuffix) } }
// clang-format on

// clang-format off
const IconMapping ICON_RESOURCE_MAPPINGS_COLOR = {
#pragma warning(push)
#pragma warning(disable : 4003) // not enough actual parameters for macro 'identifier'
	ICON_RESOURCE_MAPPINGS()
#pragma warning(pop)
};
// clang-format on

// clang-format off
const IconMapping ICON_RESOURCE_MAPPINGS_WINDOWS_10 = {
	ICON_RESOURCE_MAPPINGS(_WINDOWS_10)
};
// clang-format on

// clang-format off
const IconMapping ICON_RESOURCE_MAPPINGS_FLUENT_UI = {
	ICON_RESOURCE_MAPPINGS(_FLUENT_UI)
};
// clang-format on