// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "../Helper/ImageHelper.h"
#include <map>
#include <unordered_map>

wil::unique_hbitmap RetrieveBitmapFromGdiplusBitmap(Gdiplus::Bitmap *gdiplusBitmap);
wil::unique_hicon RetrieveIconFromGdiplusBitmap(Gdiplus::Bitmap *gdiplusBitmap);

const std::unordered_map<Icon, std::map<int, UINT>> ICON_RESOURCE_MAPPINGS = {
	{
		Icon::AddBookmark,
		{
			{16, IDB_ADD_BOOKMARK_16},
			{24, IDB_ADD_BOOKMARK_24},
			{32, IDB_ADD_BOOKMARK_32},
			{48, IDB_ADD_BOOKMARK_48}
		}
	},
	{
		Icon::ArrowRight,
		{
			{16, IDB_ARROW_RIGHT_16},
			{24, IDB_ARROW_RIGHT_24},
			{32, IDB_ARROW_RIGHT_32},
			{48, IDB_ARROW_RIGHT_48}
		}
	},
	{
		Icon::Back,
		{
			{16, IDB_BACK_16},
			{24, IDB_BACK_24},
			{32, IDB_BACK_32},
			{48, IDB_BACK_48}
		}
	},
	{
		Icon::Bookmarks,
		{
			{16, IDB_BOOKMARKS_16},
			{24, IDB_BOOKMARKS_24},
			{32, IDB_BOOKMARKS_32},
			{48, IDB_BOOKMARKS_48}
		}
	},
	{
		Icon::CloseButton,
		{
			{16, IDB_CLOSE_BUTTON_16},
			{24, IDB_CLOSE_BUTTON_24},
			{32, IDB_CLOSE_BUTTON_32},
			{48, IDB_CLOSE_BUTTON_48}
		}
	},
	{
		Icon::CloseTab,
		{
			{16, IDB_CLOSE_TAB_16},
			{24, IDB_CLOSE_TAB_24},
			{32, IDB_CLOSE_TAB_32},
			{48, IDB_CLOSE_TAB_48}
		}
	},
	{
		Icon::CommandLine,
		{
			{16, IDB_COMMAND_LINE_16},
			{24, IDB_COMMAND_LINE_24},
			{32, IDB_COMMAND_LINE_32},
			{48, IDB_COMMAND_LINE_48}
		}
	},
	{
		Icon::CommandLineAdmin,
		{
			{16, IDB_COMMAND_LINE_ADMIN_16},
			{24, IDB_COMMAND_LINE_ADMIN_24},
			{32, IDB_COMMAND_LINE_ADMIN_32},
			{48, IDB_COMMAND_LINE_ADMIN_48}
		}
	},
	{
		Icon::Copy,
		{
			{16, IDB_COPY_16},
			{24, IDB_COPY_24},
			{32, IDB_COPY_32},
			{48, IDB_COPY_48}
		}
	},
	{
		Icon::CopyTo,
		{
			{16, IDB_COPY_TO_16},
			{24, IDB_COPY_TO_24},
			{32, IDB_COPY_TO_32},
			{48, IDB_COPY_TO_48}
		}
	},
	{
		Icon::CustomizeColors,
		{
			{16, IDB_CUSTOMIZE_COLORS_16},
			{24, IDB_CUSTOMIZE_COLORS_24},
			{32, IDB_CUSTOMIZE_COLORS_32},
			{48, IDB_CUSTOMIZE_COLORS_48}
		}
	},
	{
		Icon::Cut,
		{
			{16, IDB_CUT_16},
			{24, IDB_CUT_24},
			{32, IDB_CUT_32},
			{48, IDB_CUT_48}
		}
	},
	{
		Icon::Delete,
		{
			{16, IDB_DELETE_16},
			{24, IDB_DELETE_24},
			{32, IDB_DELETE_32},
			{48, IDB_DELETE_48}
		}
	},
	{
		Icon::DeletePermanently,
		{
			{16, IDB_DELETE_PERMANENTLY_16},
			{24, IDB_DELETE_PERMANENTLY_24},
			{32, IDB_DELETE_PERMANENTLY_32},
			{48, IDB_DELETE_PERMANENTLY_48}
		}
	},
	{
		Icon::Filter,
		{
			{16, IDB_FILTER_16},
			{24, IDB_FILTER_24},
			{32, IDB_FILTER_32},
			{48, IDB_FILTER_48}
		}
	},
	{
		Icon::Folder,
		{
			{16, IDB_FOLDER_16},
			{24, IDB_FOLDER_24},
			{32, IDB_FOLDER_32},
			{48, IDB_FOLDER_48}
		}
	},
	{
		Icon::FolderTree,
		{
			{16, IDB_FOLDER_TREE_16},
			{24, IDB_FOLDER_TREE_24},
			{32, IDB_FOLDER_TREE_32},
			{48, IDB_FOLDER_TREE_48}
		}
	},
	{
		Icon::Forward,
		{
			{16, IDB_FORWARD_16},
			{24, IDB_FORWARD_24},
			{32, IDB_FORWARD_32},
			{48, IDB_FORWARD_48}
		}
	},
	{
		Icon::Help,
		{
			{16, IDB_HELP_16},
			{24, IDB_HELP_24},
			{32, IDB_HELP_32},
			{48, IDB_HELP_48}
		}
	},
	{
		Icon::Lock,
		{
			{16, IDB_LOCK_16},
			{24, IDB_LOCK_24},
			{32, IDB_LOCK_32},
			{48, IDB_LOCK_48}
		}
	},
	{
		Icon::MassRename,
		{
			{16, IDB_MASS_RENAME_16},
			{24, IDB_MASS_RENAME_24},
			{32, IDB_MASS_RENAME_32},
			{48, IDB_MASS_RENAME_48}
		}
	},
	{
		Icon::MergeFiles,
		{
			{16, IDB_MERGE_FILES_16},
			{24, IDB_MERGE_FILES_24},
			{32, IDB_MERGE_FILES_32},
			{48, IDB_MERGE_FILES_48}
		}
	},
	{
		Icon::MoveTo,
		{
			{16, IDB_MOVE_TO_16},
			{24, IDB_MOVE_TO_24},
			{32, IDB_MOVE_TO_32},
			{48, IDB_MOVE_TO_48}
		}
	},
	{
		Icon::NewFolder,
		{
			{16, IDB_NEW_FOLDER_16},
			{24, IDB_NEW_FOLDER_24},
			{32, IDB_NEW_FOLDER_32},
			{48, IDB_NEW_FOLDER_48}
		}
	},
	{
		Icon::NewTab,
		{
			{16, IDB_NEW_TAB_16},
			{24, IDB_NEW_TAB_24},
			{32, IDB_NEW_TAB_32},
			{48, IDB_NEW_TAB_48}
		}
	},
	{
		Icon::Options,
		{
			{16, IDB_OPTIONS_16},
			{24, IDB_OPTIONS_24},
			{32, IDB_OPTIONS_32},
			{48, IDB_OPTIONS_48}
		}
	},
	{
		Icon::Paste,
		{
			{16, IDB_PASTE_16},
			{24, IDB_PASTE_24},
			{32, IDB_PASTE_32},
			{48, IDB_PASTE_48}
		}
	},
	{
		Icon::PasteShortcut,
		{
			{16, IDB_PASTE_SHORTCUT_16},
			{24, IDB_PASTE_SHORTCUT_24},
			{32, IDB_PASTE_SHORTCUT_32},
			{48, IDB_PASTE_SHORTCUT_48}
		}
	},
	{
		Icon::Properties,
		{
			{16, IDB_PROPERTIES_16},
			{24, IDB_PROPERTIES_24},
			{32, IDB_PROPERTIES_32},
			{48, IDB_PROPERTIES_48}
		}
	},
	{
		Icon::Refresh,
		{
			{16, IDB_REFRESH_16},
			{24, IDB_REFRESH_24},
			{32, IDB_REFRESH_32},
			{48, IDB_REFRESH_48}
		}
	},
	{
		Icon::Rename,
		{
			{16, IDB_RENAME_16},
			{24, IDB_RENAME_24},
			{32, IDB_RENAME_32},
			{48, IDB_RENAME_48}
		}
	},
	{
		Icon::Search,
		{
			{16, IDB_SEARCH_16},
			{24, IDB_SEARCH_24},
			{32, IDB_SEARCH_32},
			{48, IDB_SEARCH_48}
		}
	},
	{
		Icon::SelectColumns,
		{
			{16, IDB_SELECT_COLUMNS_16},
			{24, IDB_SELECT_COLUMNS_24},
			{32, IDB_SELECT_COLUMNS_32},
			{48, IDB_SELECT_COLUMNS_48}
		}
	},
	{
		Icon::SplitFiles,
		{
			{16, IDB_SPLIT_FILES_16},
			{24, IDB_SPLIT_FILES_24},
			{32, IDB_SPLIT_FILES_32},
			{48, IDB_SPLIT_FILES_48}
		}
	},
	{
		Icon::Undo,
		{
			{16, IDB_UNDO_16},
			{24, IDB_UNDO_24},
			{32, IDB_UNDO_32},
			{48, IDB_UNDO_48}
		}
	},
	{
		Icon::Up,
		{
			{16, IDB_UP_16},
			{24, IDB_UP_24},
			{32, IDB_UP_32},
			{48, IDB_UP_48}
		}
	},
	{
		Icon::Views,
		{
			{16, IDB_VIEWS_16},
			{24, IDB_VIEWS_24},
			{32, IDB_VIEWS_32},
			{48, IDB_VIEWS_48}
		}
	}
};

wil::unique_hbitmap IconResourceLoader::LoadBitmapFromPNGForDpi(Icon icon, int iconWidth, int iconHeight, int dpi)
{
	auto gdiplusBitmap = LoadGdiplusBitmapFromPNGForDpi(icon, iconWidth, iconHeight, dpi);
	return RetrieveBitmapFromGdiplusBitmap(gdiplusBitmap.get());
}

wil::unique_hbitmap IconResourceLoader::LoadBitmapFromPNGAndScale(Icon icon, int iconWidth, int iconHeight)
{
	auto gdiplusBitmap = LoadGdiplusBitmapFromPNGAndScale(icon, iconWidth, iconHeight);
	return RetrieveBitmapFromGdiplusBitmap(gdiplusBitmap.get());
}

wil::unique_hbitmap RetrieveBitmapFromGdiplusBitmap(Gdiplus::Bitmap *gdiplusBitmap)
{
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

wil::unique_hicon IconResourceLoader::LoadIconFromPNGForDpi(Icon icon, int iconWidth, int iconHeight, int dpi)
{
	auto gdiplusBitmap = LoadGdiplusBitmapFromPNGForDpi(icon, iconWidth, iconHeight, dpi);
	return RetrieveIconFromGdiplusBitmap(gdiplusBitmap.get());
}

wil::unique_hicon IconResourceLoader::LoadIconFromPNGAndScale(Icon icon, int iconWidth, int iconHeight)
{
	auto gdiplusBitmap = LoadGdiplusBitmapFromPNGAndScale(icon, iconWidth, iconHeight);
	return RetrieveIconFromGdiplusBitmap(gdiplusBitmap.get());
}

wil::unique_hicon RetrieveIconFromGdiplusBitmap(Gdiplus::Bitmap *gdiplusBitmap)
{
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

std::unique_ptr<Gdiplus::Bitmap> IconResourceLoader::LoadGdiplusBitmapFromPNGForDpi(Icon icon, int iconWidth, int iconHeight, int dpi)
{
	int scaledIconWidth = MulDiv(iconWidth, dpi, USER_DEFAULT_SCREEN_DPI);
	int scaledIconHeight = MulDiv(iconHeight, dpi, USER_DEFAULT_SCREEN_DPI);
	return LoadGdiplusBitmapFromPNGAndScale(icon, scaledIconWidth, scaledIconHeight);
}

// This function is based on the steps performed by https://docs.microsoft.com/en-us/windows/win32/api/commctrl/nf-commctrl-loadiconmetric
// when loading an icon (see the remarks section on that page for details).
std::unique_ptr<Gdiplus::Bitmap> IconResourceLoader::LoadGdiplusBitmapFromPNGAndScale(Icon icon, int iconWidth, int iconHeight)
{
	const auto &iconSizeMappins = ICON_RESOURCE_MAPPINGS.at(icon);

	auto match = std::find_if(iconSizeMappins.begin(), iconSizeMappins.end(),
		[iconWidth, iconHeight] (auto entry) {
			return iconWidth <= entry.first && iconHeight <= entry.first;
		}
	);

	if (match == iconSizeMappins.end())
	{
		match = std::prev(iconSizeMappins.end());
	}

	auto bitmap = ImageHelper::LoadGdiplusBitmapFromPNG(GetModuleHandle(nullptr), match->second);

	// If the icon size matches exactly, it doesn't need to be scaled, so can be
	// returned immediately.
	if (match->first == iconWidth
		&& match->first == iconHeight)
	{
		return bitmap;
	}

	auto scaledBitmap = std::make_unique<Gdiplus::Bitmap>(iconWidth, iconHeight);
	Gdiplus::Graphics graphics(scaledBitmap.get());

	float scalingFactorX = static_cast<float>(iconWidth) / static_cast<float>(match->first);
	float scalingFactorY = static_cast<float>(iconHeight) / static_cast<float>(match->first);
	graphics.ScaleTransform(scalingFactorX, scalingFactorY);
	graphics.DrawImage(bitmap.get(), 0, 0);

	return scaledBitmap;
}