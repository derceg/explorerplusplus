// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/DataObjectImpl.h"
#include "../Helper/DataExchangeHelper.h"
#include "../Helper/DragDropHelper.h"
#include <gtest/gtest.h>
#include <wil/com.h>

using namespace testing;

class DataObjectImplTest : public Test
{
protected:
	DataObjectImplTest()
	{
		m_dataObject = winrt::make_self<DataObjectImpl>(nullptr, nullptr, 0);
	}

	winrt::com_ptr<IDataObject> m_dataObject;
};

TEST_F(DataObjectImplTest, QueryInterface)
{
	wil::com_ptr_nothrow<IUnknown> unknown;
	HRESULT hr = m_dataObject->QueryInterface(IID_PPV_ARGS(&unknown));
	EXPECT_HRESULT_SUCCEEDED(hr);

	wil::com_ptr_nothrow<IDataObject> dataObject;
	hr = m_dataObject->QueryInterface(IID_PPV_ARGS(&dataObject));
	EXPECT_HRESULT_SUCCEEDED(hr);
}

TEST_F(DataObjectImplTest, QueryGetDataEmpty)
{
	FORMATETC formatEtc = { CF_TEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	HRESULT hr = m_dataObject->QueryGetData(&formatEtc);
	EXPECT_EQ(DV_E_FORMATETC, hr);
}

TEST_F(DataObjectImplTest, GetDataEmpty)
{
	FORMATETC formatEtc = { CF_TEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stgMedium;
	HRESULT hr = m_dataObject->GetData(&formatEtc, &stgMedium);
	EXPECT_EQ(DV_E_FORMATETC, hr);
}

TEST_F(DataObjectImplTest, SetDataAndGetData)
{
	FORMATETC formatEtc = { CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

	auto global = WriteStringToGlobal(L"Test text");
	STGMEDIUM stgMedium = GetStgMediumForGlobal(global.get());

	HRESULT hr = m_dataObject->SetData(&formatEtc, &stgMedium, FALSE);
	ASSERT_HRESULT_SUCCEEDED(hr);

	global.release();

	STGMEDIUM stgMediumOut;
	hr = m_dataObject->GetData(&formatEtc, &stgMediumOut);
	ASSERT_HRESULT_SUCCEEDED(hr);

	ReleaseStgMedium(&stgMediumOut);
}
