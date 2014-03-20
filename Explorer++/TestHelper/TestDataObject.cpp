#include "stdafx.h"
#include "gtest\gtest.h"
#include "../Helper/iDataObject.h"
#include "../Helper/Macros.h"

void CreateEmptyDataObject(IDataObject **pDataObject)
{
	HRESULT hr = CreateDataObject(NULL, NULL, pDataObject, 0);
	ASSERT_TRUE(SUCCEEDED(hr));
}

class iDataObjectTest : public ::testing::Test
{
protected:

	void SetUp()
	{
		CreateEmptyDataObject(&m_pDataObject);
	}

	void TearDown()
	{
		m_pDataObject->Release();
	}

	IDataObject *m_pDataObject;
};

TEST_F(iDataObjectTest, QueryInterface)
{
	IUnknown *pUnknownOut = NULL;
	HRESULT hr = m_pDataObject->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&pUnknownOut));
	EXPECT_EQ(S_OK, hr);

	pUnknownOut->Release();

	IDataObject *pDataObjectOut = NULL;
	hr = m_pDataObject->QueryInterface(IID_IDataObject, reinterpret_cast<void **>(&pDataObjectOut));
	EXPECT_EQ(S_OK, hr);

	pDataObjectOut->Release();
}

TEST_F(iDataObjectTest, QueryGetDataEmpty)
{
	FORMATETC ftc;
	ftc.cfFormat = CF_TEXT;
	ftc.dwAspect = DVASPECT_CONTENT;
	ftc.lindex = -1;
	ftc.ptd = NULL;
	ftc.tymed = TYMED_HGLOBAL;
	HRESULT hr = m_pDataObject->QueryGetData(&ftc);
	EXPECT_EQ(DV_E_FORMATETC, hr);
}

TEST_F(iDataObjectTest, GetDataEmpty)
{
	FORMATETC ftc;
	ftc.cfFormat = CF_TEXT;
	ftc.dwAspect = DVASPECT_CONTENT;
	ftc.lindex = -1;
	ftc.ptd = NULL;
	ftc.tymed = TYMED_HGLOBAL;
	STGMEDIUM stg;
	HRESULT hr = m_pDataObject->GetData(&ftc, &stg);
	EXPECT_EQ(DV_E_FORMATETC, hr);
}

TEST_F(iDataObjectTest, SetDataAndGetData)
{
	FORMATETC ftc;
	ftc.cfFormat = CF_TEXT;
	ftc.dwAspect = DVASPECT_CONTENT;
	ftc.lindex = -1;
	ftc.ptd = NULL;
	ftc.tymed = TYMED_HGLOBAL;

	TCHAR szText[] = _T("Test text");
	size_t textSize = (lstrlen(szText) + 1) * sizeof(TCHAR);
	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, textSize);
	ASSERT_NE(nullptr, hGlobal);

	LPVOID pMem = GlobalLock(hGlobal);
	ASSERT_NE(nullptr, pMem);

	memcpy(pMem, szText, textSize);
	GlobalUnlock(hGlobal);

	STGMEDIUM stg;
	stg.tymed = TYMED_HGLOBAL;
	stg.pUnkForRelease = NULL;
	stg.hGlobal = hGlobal;

	HRESULT hr = m_pDataObject->SetData(&ftc, &stg, FALSE);
	EXPECT_EQ(S_OK, hr);

	STGMEDIUM stgOut;
	hr = m_pDataObject->GetData(&ftc, &stgOut);
	EXPECT_EQ(S_OK, hr);

	if(stgOut.pUnkForRelease != NULL)
	{
		stgOut.pUnkForRelease->Release();
	}
	else
	{
		ReleaseStgMedium(&stgOut);
	}

	GlobalFree(hGlobal);
}