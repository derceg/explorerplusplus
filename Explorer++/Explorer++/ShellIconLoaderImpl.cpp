// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellIconLoaderImpl.h"
#include "AsyncIconFetcher.h"
#include "RuntimeHelper.h"
#include "../Helper/ImageHelper.h"
#include "../Helper/ShellHelper.h"

ShellIconLoaderImpl::ShellIconLoaderImpl(const Runtime *runtime, AsyncIconFetcher *iconFetcher) :
	m_runtime(runtime),
	m_iconFetcher(iconFetcher)
{
	FAIL_FAST_IF_FAILED(SHGetImageList(SHIL_SYSSMALL, IID_PPV_ARGS(&m_systemImageList)));
}

wil::com_ptr_nothrow<IWICBitmapSource> ShellIconLoaderImpl::LoadShellIcon(PCIDLIST_ABSOLUTE pidl,
	IconSize size, IconUpdateCallback updateCallback, std::stop_token stopToken)
{
	if (size != IconSize::Small)
	{
		return nullptr;
	}

	// The item may not have a cached icon, or the cached icon may be out of date. Therefore, this
	// call will retrieve the updated icon.
	RetrieveIcon(m_weakPtrFactory.GetWeakPtr(), pidl, std::move(updateCallback), stopToken);

	return GetDefaultIcon(pidl);
}

concurrencpp::null_result ShellIconLoaderImpl::RetrieveIcon(WeakPtr<ShellIconLoaderImpl> weakSelf,
	PidlAbsolute pidl, IconUpdateCallback updateCallback, std::stop_token stopToken)
{
	auto *runtime = weakSelf->m_runtime;
	auto iconFetcher = weakSelf->m_iconFetcher;

	auto iconInfo = co_await iconFetcher->GetIconIndexAsync(pidl.Raw(), stopToken);

	if (!iconInfo)
	{
		co_return;
	}

	co_await ResumeOnUiThread(runtime);

	if (stopToken.stop_requested() || !weakSelf)
	{
		co_return;
	}

	wil::com_ptr_nothrow<IWICBitmapSource> bitmap;
	HRESULT hr = ImageHelper::CreateBitmapFromImageListIcon(weakSelf->m_systemImageList.get(),
		iconInfo->iconIndex, bitmap);

	if (FAILED(hr))
	{
		co_return;
	}

	updateCallback(bitmap.get());
}

wil::com_ptr_nothrow<IWICBitmapSource> ShellIconLoaderImpl::GetDefaultIcon(PCIDLIST_ABSOLUTE pidl)
{
	int iconIndex = m_iconFetcher->GetCachedIconIndexOrDefault(pidl);

	wil::com_ptr_nothrow<IWICBitmapSource> bitmap;
	HRESULT hr =
		ImageHelper::CreateBitmapFromImageListIcon(m_systemImageList.get(), iconIndex, bitmap);

	if (FAILED(hr))
	{
		return nullptr;
	}

	return bitmap;
}
