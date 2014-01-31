#include "stdafx.h"
#include "gtest\gtest.h"
#include "../Helper/iDataObject.h"
#include "../Helper/Macros.h"

TEST(iDataObject, QueryInterface)
{
	IDataObject *pDataObject = NULL;
	HRESULT hr = CreateDataObject(NULL, NULL, &pDataObject, 0);
	ASSERT_TRUE(SUCCEEDED(hr));

	IDataObject *pOut = NULL;
	hr = pDataObject->QueryInterface(IID_IDataObject, reinterpret_cast<void **>(&pOut));
	EXPECT_EQ(S_OK, hr);
}

TEST(iDataObject, QueryGetData)
{
	IDataObject *pDataObject = NULL;
	HRESULT hr = CreateDataObject(NULL, NULL, &pDataObject, 0);
	ASSERT_TRUE(SUCCEEDED(hr));

	FORMATETC ftc;
	ftc.cfFormat = CF_TEXT;
	ftc.dwAspect = DVASPECT_CONTENT;
	ftc.lindex = -1;
	ftc.ptd = NULL;
	ftc.tymed = TYMED_HGLOBAL;
	hr = pDataObject->QueryGetData(&ftc);
	EXPECT_EQ(DV_E_FORMATETC, hr);
}