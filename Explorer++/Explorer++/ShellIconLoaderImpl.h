// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellIconLoader.h"
#include "../Helper/Pidl.h"
#include "../Helper/WeakPtrFactory.h"
#include <concurrencpp/concurrencpp.h>
#include <wil/com.h>

class AsyncIconFetcher;
class Runtime;

class ShellIconLoaderImpl : public ShellIconLoader
{
public:
	ShellIconLoaderImpl(const Runtime *runtime, AsyncIconFetcher *iconFetcher);

	wil::com_ptr_nothrow<IWICBitmapSource> LoadShellIcon(PCIDLIST_ABSOLUTE pidl, IconSize size,
		IconUpdateCallback updateCallback, std::stop_token stopToken) override;

private:
	static concurrencpp::null_result RetrieveIcon(WeakPtr<ShellIconLoaderImpl> weakSelf,
		PidlAbsolute pidl, IconUpdateCallback updateCallback, std::stop_token stopToken);
	wil::com_ptr_nothrow<IWICBitmapSource> GetDefaultIcon(PCIDLIST_ABSOLUTE pidl);

	const Runtime *const m_runtime;
	AsyncIconFetcher *const m_iconFetcher;
	wil::com_ptr_nothrow<IImageList> m_systemImageList;

	WeakPtrFactory<ShellIconLoaderImpl> m_weakPtrFactory{ this };
};
