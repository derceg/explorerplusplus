// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ClipboardHelper.h"
#include "BulkClipboardWriter.h"
#include "DragDropHelper.h"
#include <boost/algorithm/string/join.hpp>
#include <wil/com.h>
#include <optional>

namespace
{

std::optional<std::wstring> GetParsingPath(const PidlAbsolute &pidl)
{
	wil::unique_cotaskmem_string path;
	HRESULT hr = SHGetNameFromIDList(pidl.Raw(), SIGDN_DESKTOPABSOLUTEPARSING, &path);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	return path.get();
}

std::optional<std::wstring> GetUniversalPath(const PidlAbsolute &pidl)
{
	wil::unique_cotaskmem_string path;
	HRESULT hr = SHGetNameFromIDList(pidl.Raw(), SIGDN_DESKTOPABSOLUTEPARSING, &path);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	DWORD size = sizeof(UNIVERSAL_NAME_INFO);
	auto nameInfo = MakeUniqueVariableSizeStruct<UNIVERSAL_NAME_INFO>(size);
	DWORD res = WNetGetUniversalName(path.get(), UNIVERSAL_NAME_INFO_LEVEL, nameInfo.get(), &size);

	if (res == ERROR_MORE_DATA)
	{
		// If the item has a universal name, it's expected this branch will be taken, since the
		// original size allocated isn't large enough to hold anything.
		nameInfo = MakeUniqueVariableSizeStruct<UNIVERSAL_NAME_INFO>(size);
		res = WNetGetUniversalName(path.get(), UNIVERSAL_NAME_INFO_LEVEL, nameInfo.get(), &size);
	}

	if (res != NO_ERROR)
	{
		return path.get();
	}

	return nameInfo->lpUniversalName;
}

std::optional<std::wstring> GetPathOfType(const PidlAbsolute &pidl, PathType pathType)
{
	switch (pathType)
	{
	case PathType::Parsing:
		return GetParsingPath(pidl);

	case PathType::UniversalPath:
		return GetUniversalPath(pidl);
	}

	DCHECK(false);
	return std::nullopt;
}

}

bool CanShellPasteDataObject(PCIDLIST_ABSOLUTE destination, IDataObject *dataObject,
	PasteType pasteType)
{
	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	HRESULT hr = SHBindToObject(nullptr, destination, nullptr, IID_PPV_ARGS(&shellFolder));

	if (FAILED(hr))
	{
		return false;
	}

	wil::com_ptr_nothrow<IDropTarget> dropTarget;
	hr = shellFolder->CreateViewObject(nullptr, IID_PPV_ARGS(&dropTarget));

	if (FAILED(hr))
	{
		return false;
	}

	DWORD allowedEffects = DROPEFFECT_NONE;

	switch (pasteType)
	{
	case PasteType::Normal:
		allowedEffects = DROPEFFECT_COPY | DROPEFFECT_MOVE;
		break;

	case PasteType::Shortcut:
		allowedEffects = DROPEFFECT_LINK;
		break;
	}

	// The value below indicates whether the source prefers a copy or a move and will be used to try
	// and determine whether a paste operation would succeed. This is important for folders like the
	// recycle bin, where it is possible to paste, though only if the items are moved.
	DWORD preferredEffect;
	hr = GetPreferredDropEffect(dataObject, preferredEffect);

	if (FAILED(hr))
	{
		// If no preferred effect is set, assume that the source prefers a copy.
		preferredEffect = DROPEFFECT_COPY | DROPEFFECT_LINK;
	}

	allowedEffects &= preferredEffect;

	// Internally, a paste is a simulated drop, so this should give a reliable indication of whether
	// such a drop would succeed.
	// Note that this can cause the COM modal loop to run and PeekMessage() to be called. In that
	// case, certain window messages, like WM_CLOSE, can be dispatched.
	DWORD finalEffect = allowedEffects;
	hr = dropTarget->DragEnter(dataObject, MK_LBUTTON, { 0, 0 }, &finalEffect);

	if (FAILED(hr) || finalEffect == DROPEFFECT_NONE)
	{
		return false;
	}

	return true;
}

void CopyItemPathsToClipboard(ClipboardStore *clipboardStore,
	const std::vector<PidlAbsolute> &items, PathType pathType)
{
	std::vector<std::wstring> paths;

	for (const auto &item : items)
	{
		auto path = GetPathOfType(item, pathType);

		if (!path)
		{
			continue;
		}

		paths.push_back(*path);
	}

	if (paths.empty())
	{
		return;
	}

	BulkClipboardWriter clipboardWriter(clipboardStore);
	clipboardWriter.WriteText(boost::algorithm::join(paths, L"\r\n"));
}
