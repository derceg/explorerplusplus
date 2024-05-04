// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "DragDropTestHelper.h"
#include "ShellTestHelper.h"
#include "../Helper/DataExchangeHelper.h"
#include "../Helper/DataObjectImpl.h"
#include "../Helper/DragDropHelper.h"
#include <gtest/gtest.h>

void CreateTextDataObject(const std::wstring &text, winrt::com_ptr<IDataObject> &dataObject)
{
	FORMATETC formatEtc = { CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

	auto global = WriteStringToGlobal(text);
	ASSERT_TRUE(global.is_valid());

	auto stgMedium = GetStgMediumForGlobal(std::move(global));

	dataObject = winrt::make<DataObjectImpl>();
	ASSERT_HRESULT_SUCCEEDED(
		MoveStorageToObject(dataObject.get(), &formatEtc, std::move(stgMedium)));
}

void CreateShellDataObject(const std::wstring &path, ShellItemType shellItemType,
	wil::com_ptr_nothrow<IDataObject> &dataObject)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(path, nullptr, shellItemType);
	std::vector<PCIDLIST_ABSOLUTE> items = { pidl.Raw() };
	ASSERT_HRESULT_SUCCEEDED(CreateDataObjectForShellTransfer(items, &dataObject));
}
