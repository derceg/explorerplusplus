// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "../Helper/ImageHelper.h"
#include <map>
#include <unordered_map>

const std::unordered_map<Icon, std::map<int, UINT>> ICON_RESOURCE_MAPPINGS = {
	{
		Icon::AddBookmark,
		{
			{16, IDB_ADD_BOOKMARK_16},
			{24, IDB_ADD_BOOKMARK_24}
		}
	},
	{
		Icon::ArrowRight,
		{
			{16, IDB_ARROW_RIGHT_16},
			{24, IDB_ARROW_RIGHT_24}
		}
	},
	{
		Icon::Back,
		{
			{16, IDB_BACK_16},
			{24, IDB_BACK_24}
		}
	},
	{
		Icon::Bookmarks,
		{
			{16, IDB_BOOKMARKS_16},
			{24, IDB_BOOKMARKS_24}
		}
	},
	{
		Icon::CloseButton,
		{
			{16, IDB_CLOSE_BUTTON_16},
			{24, IDB_CLOSE_BUTTON_24}
		}
	},
	{
		Icon::CloseTab,
		{
			{16, IDB_CLOSE_TAB_16},
			{24, IDB_CLOSE_TAB_24}
		}
	},
	{
		Icon::CommandLine,
		{
			{16, IDB_COMMAND_LINE_16},
			{24, IDB_COMMAND_LINE_24}
		}
	},
	{
		Icon::CommandLineAdmin,
		{
			{16, IDB_COMMAND_LINE_ADMIN_16},
			{24, IDB_COMMAND_LINE_ADMIN_24}
		}
	},
	{
		Icon::Copy,
		{
			{16, IDB_COPY_16},
			{24, IDB_COPY_24}
		}
	},
	{
		Icon::CopyTo,
		{
			{16, IDB_COPY_TO_16},
			{24, IDB_COPY_TO_24}
		}
	},
	{
		Icon::CustomizeColors,
		{
			{16, IDB_CUSTOMIZE_COLORS_16},
			{24, IDB_CUSTOMIZE_COLORS_24}
		}
	},
	{
		Icon::Cut,
		{
			{16, IDB_CUT_16},
			{24, IDB_CUT_24}
		}
	},
	{
		Icon::Delete,
		{
			{16, IDB_DELETE_16},
			{24, IDB_DELETE_24}
		}
	},
	{
		Icon::DeletePermanently,
		{
			{16, IDB_DELETE_PERMANENTLY_16},
			{24, IDB_DELETE_PERMANENTLY_24}
		}
	},
	{
		Icon::Filter,
		{
			{16, IDB_FILTER_16},
			{24, IDB_FILTER_24}
		}
	},
	{
		Icon::Folder,
		{
			{16, IDB_FOLDER_16},
			{24, IDB_FOLDER_24}
		}
	},
	{
		Icon::FolderTree,
		{
			{16, IDB_FOLDER_TREE_16},
			{24, IDB_FOLDER_TREE_24}
		}
	},
	{
		Icon::Forward,
		{
			{16, IDB_FORWARD_16},
			{24, IDB_FORWARD_24}
		}
	},
	{
		Icon::Help,
		{
			{16, IDB_HELP_16},
			{24, IDB_HELP_24}
		}
	},
	{
		Icon::Lock,
		{
			{16, IDB_LOCK_16},
			{24, IDB_LOCK_24}
		}
	},
	{
		Icon::MassRename,
		{
			{16, IDB_MASS_RENAME_16},
			{24, IDB_MASS_RENAME_24}
		}
	},
	{
		Icon::MergeFiles,
		{
			{16, IDB_MERGE_FILES_16},
			{24, IDB_MERGE_FILES_24}
		}
	},
	{
		Icon::MoveTo,
		{
			{16, IDB_MOVE_TO_16},
			{24, IDB_MOVE_TO_24}
		}
	},
	{
		Icon::NewFolder,
		{
			{16, IDB_NEW_FOLDER_16},
			{24, IDB_NEW_FOLDER_24}
		}
	},
	{
		Icon::NewTab,
		{
			{16, IDB_NEW_TAB_16},
			{24, IDB_NEW_TAB_24}
		}
	},
	{
		Icon::Options,
		{
			{16, IDB_OPTIONS_16},
			{24, IDB_OPTIONS_24}
		}
	},
	{
		Icon::Paste,
		{
			{16, IDB_PASTE_16},
			{24, IDB_PASTE_24}
		}
	},
	{
		Icon::PasteShortcut,
		{
			{16, IDB_PASTE_SHORTCUT_16},
			{24, IDB_PASTE_SHORTCUT_24}
		}
	},
	{
		Icon::Properties,
		{
			{16, IDB_PROPERTIES_16},
			{24, IDB_PROPERTIES_24}
		}
	},
	{
		Icon::Refresh,
		{
			{16, IDB_REFRESH_16},
			{24, IDB_REFRESH_24}
		}
	},
	{
		Icon::Rename,
		{
			{16, IDB_RENAME_16},
			{24, IDB_RENAME_24}
		}
	},
	{
		Icon::Search,
		{
			{16, IDB_SEARCH_16},
			{24, IDB_SEARCH_24}
		}
	},
	{
		Icon::SelectColumns,
		{
			{16, IDB_SELECT_COLUMNS_16},
			{24, IDB_SELECT_COLUMNS_24}
		}
	},
	{
		Icon::SplitFiles,
		{
			{16, IDB_SPLIT_FILES_16},
			{24, IDB_SPLIT_FILES_24}
		}
	},
	{
		Icon::Undo,
		{
			{16, IDB_UNDO_16},
			{24, IDB_UNDO_24}
		}
	},
	{
		Icon::Up,
		{
			{16, IDB_UP_16},
			{24, IDB_UP_24}
		}
	},
	{
		Icon::Views,
		{
			{16, IDB_VIEWS_16},
			{24, IDB_VIEWS_24}
		}
	}
};

wil::unique_hbitmap IconResourceLoader::LoadBitmapFromPNGForDpi(Icon icon, int iconSize, int dpi)
{
	auto gdiplusBitmap = LoadGdiplusBitmapFromPNGForDpi(icon, iconSize, dpi);

	if (!gdiplusBitmap)
	{
		return nullptr;
	}

	wil::unique_hbitmap bitmap;
	Gdiplus::Color color(0, 0, 0);
	Gdiplus::Status status = gdiplusBitmap->GetHBITMAP(color, &bitmap);

	if (status != Gdiplus::Status::Ok)
	{
		return nullptr;
	}

	return bitmap;
}

wil::unique_hicon IconResourceLoader::LoadIconFromPNGForDpi(Icon icon, int iconSize, int dpi)
{
	auto gdiplusBitmap = LoadGdiplusBitmapFromPNGForDpi(icon, iconSize, dpi);

	if (!gdiplusBitmap)
	{
		return nullptr;
	}

	wil::unique_hicon hicon;
	Gdiplus::Status status = gdiplusBitmap->GetHICON(&hicon);

	if (status != Gdiplus::Status::Ok)
	{
		return nullptr;
	}

	return hicon;
}

// This function is based on the steps performed by https://docs.microsoft.com/en-us/windows/win32/api/commctrl/nf-commctrl-loadiconmetric
// when loading an icon (see the remarks section on that page for details).
std::unique_ptr<Gdiplus::Bitmap> IconResourceLoader::LoadGdiplusBitmapFromPNGForDpi(Icon icon, int iconSize, int dpi)
{
	const auto &iconSizeMappins = ICON_RESOURCE_MAPPINGS.at(icon);
	int scaledIconSize = MulDiv(iconSize, dpi, USER_DEFAULT_SCREEN_DPI);

	auto match = std::find_if(iconSizeMappins.begin(), iconSizeMappins.end(),
		[scaledIconSize] (auto entry) {
			return scaledIconSize <= entry.first;
		}
	);

	if (match == iconSizeMappins.end())
	{
		match = std::prev(iconSizeMappins.end());
	}

	auto bitmap = ImageHelper::LoadGdiplusBitmapFromPNG(GetModuleHandle(nullptr), match->second);

	// If the icon size matches exactly, it doesn't need to be scaled, so can be
	// returned immediately.
	if (match->first == scaledIconSize)
	{
		return bitmap;
	}

	auto scaledBitmap = std::make_unique<Gdiplus::Bitmap>(scaledIconSize, scaledIconSize);
	Gdiplus::Graphics graphics(scaledBitmap.get());

	float scalingFactor = static_cast<float>(scaledIconSize) / static_cast<float>(match->first);
	graphics.ScaleTransform(scalingFactor, scalingFactor);
	graphics.DrawImage(bitmap.get(), 0, 0);

	return scaledBitmap;
}