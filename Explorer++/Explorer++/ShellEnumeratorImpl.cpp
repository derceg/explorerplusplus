// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellEnumeratorImpl.h"
#include "../Helper/ShellHelper.h"
#include <wil/common.h>

ShellEnumeratorImpl::ShellEnumeratorImpl(HWND embedder) : m_embedder(embedder)
{
}

HRESULT ShellEnumeratorImpl::EnumerateDirectory(PCIDLIST_ABSOLUTE pidlDirectory,
	ShellItemFilter::ItemType itemType, ShellItemFilter::HiddenItemPolicy hiddenItemPolicy,
	std::vector<PidlChild> &outputItems, std::stop_token stopToken) const
{
	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	RETURN_IF_FAILED(SHBindToObject(nullptr, pidlDirectory, nullptr, IID_PPV_ARGS(&shellFolder)));

	SHCONTF enumFlags = SHCONTF_FOLDERS;

	if (itemType == ShellItemFilter::ItemType::FoldersAndFiles)
	{
		WI_SetFlag(enumFlags, SHCONTF_NONFOLDERS);
	}

	if (hiddenItemPolicy == ShellItemFilter::HiddenItemPolicy::Include)
	{
		WI_SetAllFlags(enumFlags, SHCONTF_INCLUDEHIDDEN | SHCONTF_INCLUDESUPERHIDDEN);
	}

	// Note that if this function operates asynchronously, the `m_embedder` window handle passed in
	// here could be invalidated at any time. Unfortunately, there doesn't seem to be any way to
	// deal with that. If the handle is invalid at the point where the enumerator needs to show UI,
	// the enumeration will simply fail silently. While that behavior isn't an issue, there's still
	// the general problem that window handles can be reused, so the window handle could end up
	// referring to a completely different window.
	wil::com_ptr_nothrow<IEnumIDList> enumerator;
	HRESULT hr = shellFolder->EnumObjects(m_embedder, enumFlags, &enumerator);

	if (FAILED(hr) || !enumerator)
	{
		return hr;
	}

	ULONG numFetched = 1;
	unique_pidl_child pidlItem;

	while (!stopToken.stop_requested()
		&& enumerator->Next(1, wil::out_param(pidlItem), &numFetched) == S_OK && (numFetched == 1))
	{
		outputItems.emplace_back(pidlItem.get());
	}

	return S_OK;
}
