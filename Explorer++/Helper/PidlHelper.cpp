// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PidlHelper.h"
#include "Base64Wrapper.h"
#include "Helper.h"
#include "Pidl.h"
#include "WilExtraTypes.h"
#include <wil/com.h>

namespace
{

class FileSystemBindData :
	public winrt::implements<FileSystemBindData, IFileSystemBindData, winrt::non_agile>
{
public:
	FileSystemBindData(const WIN32_FIND_DATA *findData) : m_findData(*findData)
	{
	}

	IFACEMETHODIMP SetFindData(const WIN32_FIND_DATA *findData)
	{
		m_findData = *findData;
		return S_OK;
	}

	IFACEMETHODIMP GetFindData(WIN32_FIND_DATA *findData)
	{
		*findData = m_findData;
		return S_OK;
	}

private:
	WIN32_FIND_DATA m_findData;
};

}

std::string EncodePidlToBase64(const PidlAbsolute &pidl)
{
	return cppcodec::base64_rfc4648::encode(reinterpret_cast<const char *>(pidl.Raw()),
		ILGetSize(pidl.Raw()));
}

PidlAbsolute DecodePidlFromBase64(const std::string &encodedPidl)
{
	std::vector<uint8_t> decodedContent;

	try
	{
		decodedContent = cppcodec::base64_rfc4648::decode(encodedPidl);
	}
	catch (const cppcodec::parse_error &)
	{
		return {};
	}

	auto size = decodedContent.size();

	if (size == 0)
	{
		return {};
	}

	unique_pidl_absolute pidl(static_cast<PIDLIST_ABSOLUTE>(CoTaskMemAlloc(size)));

	if (!pidl)
	{
		return {};
	}

	std::memcpy(pidl.get(), decodedContent.data(), size);

	if (!IDListContainerIsConsistent(pidl.get(), CheckedNumericCast<UINT>(size)))
	{
		return {};
	}

	return pidl.get();
}

// This performs the same function as SHSimpleIDListFromPath(), which is deprecated. The path
// provided should be relative to the parent. If parent is null, the path should be absolute.
HRESULT CreateSimplePidl(const std::wstring &path, PidlAbsolute &outputPidl, IShellFolder *parent,
	ShellItemType itemType, ShellItemExtraAttributes extraAttributes)
{
	wil::com_ptr_nothrow<IBindCtx> bindCtx;
	RETURN_IF_FAILED(CreateBindCtx(0, &bindCtx));

	BIND_OPTS opts = { sizeof(opts), 0, STGM_CREATE, 0 };
	RETURN_IF_FAILED(bindCtx->SetBindOptions(&opts));

	WIN32_FIND_DATA wfd = {};

	switch (itemType)
	{
	case ShellItemType::File:
		wfd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
		break;

	case ShellItemType::Folder:
		wfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		break;
	}

	WI_SetAllFlags(wfd.dwFileAttributes, static_cast<DWORD>(extraAttributes));

	auto fsBindData = winrt::make<FileSystemBindData>(&wfd);

	RETURN_IF_FAILED(
		bindCtx->RegisterObjectParam(const_cast<PWSTR>(STR_FILE_SYS_BIND_DATA), fsBindData.get()));

	if (!parent)
	{
		return SHParseDisplayName(path.c_str(), bindCtx.get(), PidlOutParam(outputPidl), 0,
			nullptr);
	}

	unique_pidl_relative pidlRelative;
	RETURN_IF_FAILED(parent->ParseDisplayName(nullptr, bindCtx.get(),
		const_cast<LPWSTR>(path.c_str()), nullptr, wil::out_param(pidlRelative), nullptr));

	unique_pidl_absolute pidlParent;
	RETURN_IF_FAILED(SHGetIDListFromObject(parent, wil::out_param(pidlParent)));

	outputPidl = PidlAbsolute(ILCombine(pidlParent.get(), pidlRelative.get()), Pidl::takeOwnership);

	return S_OK;
}
