#include "stdafx.h"
#include <string>
#include "../Helper/iDataObject.h"
#include "../Helper/Macros.h"
#include "gtest\gtest.h"

TEST(iDataObjectTest,Simple)
{
	IDataObject *pDataObject = NULL;
	CreateDataObject(NULL,NULL,&pDataObject,0);

	FORMATETC ftc;
	ftc.cfFormat	= CF_TEXT;
	ftc.dwAspect	= DVASPECT_CONTENT;
	ftc.lindex		= -1;
	ftc.ptd			= NULL;
	ftc.tymed		= TYMED_HGLOBAL;
	EXPECT_EQ(DV_E_FORMATETC,pDataObject->QueryGetData(&ftc));

	EXPECT_EQ(E_NOTIMPL,pDataObject->DAdvise(NULL,ADVF_NODATA,NULL,NULL));
	EXPECT_EQ(OLE_E_ADVISENOTSUPPORTED,pDataObject->DUnadvise(0));
	EXPECT_EQ(OLE_E_ADVISENOTSUPPORTED,pDataObject->EnumDAdvise(NULL));
}