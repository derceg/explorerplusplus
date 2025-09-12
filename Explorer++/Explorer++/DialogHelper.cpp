// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DialogHelper.h"
#include "SetFileAttributesDialog.h"

namespace
{

bool CanShowSetFileAttributesDialogForItem(const PidlAbsolute &pidl)
{
	wil::unique_cotaskmem_string path;
	HRESULT hr = SHGetNameFromIDList(pidl.Raw(), SIGDN_FILESYSPATH, &path);

	if (FAILED(hr))
	{
		return false;
	}

	if (PathIsRoot(path.get()))
	{
		return false;
	}

	return true;
}

}

namespace DialogHelper
{

bool CanShowSetFileAttributesDialogForItems(const std::vector<PidlAbsolute> &pidls)
{
	return !pidls.empty() && std::ranges::all_of(pidls, CanShowSetFileAttributesDialogForItem);
}

void MaybeShowSetFileAttributesDialog(const ResourceLoader *resourceLoader, HWND parent,
	const std::vector<ItemPidlAndFindData> &items)
{
	std::vector<SetFileAttributesItem> dialogItems;

	for (const auto &item : items)
	{
		if (!CanShowSetFileAttributesDialogForItem(item.pidl))
		{
			return;
		}

		wil::unique_cotaskmem_string path;
		HRESULT hr = SHGetNameFromIDList(item.pidl.Raw(), SIGDN_FILESYSPATH, &path);

		if (FAILED(hr))
		{
			return;
		}

		WIN32_FIND_DATA findData;

		if (item.findData)
		{
			findData = *item.findData;
		}
		else
		{
			wil::unique_hfind findFile(FindFirstFile(path.get(), &findData));

			if (!findFile)
			{
				return;
			}
		}

		dialogItems.emplace_back(path.get(), findData);
	}

	if (dialogItems.empty())
	{
		return;
	}

	auto *setFileAttributesDialog =
		SetFileAttributesDialog::Create(resourceLoader, parent, dialogItems);
	setFileAttributesDialog->ShowModalDialog();
}

}
