// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DragDropHelper.h"
#include "DataExchangeHelper.h"
#include <wil/com.h>

STGMEDIUM GetStgMediumForGlobal(HGLOBAL global)
{
	STGMEDIUM storage;
	storage.tymed = TYMED_HGLOBAL;
	storage.hGlobal = global;
	storage.pUnkForRelease = nullptr;
	return storage;
}

STGMEDIUM GetStgMediumForStream(IStream *stream)
{
	STGMEDIUM storage;
	storage.tymed = TYMED_ISTREAM;
	storage.pstm = stream;
	storage.pUnkForRelease = nullptr;
	return storage;
}

HRESULT SetPreferredDropEffect(IDataObject *dataObject, DWORD effect)
{
	FORMATETC ftc = { static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT)),
		nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

	auto global = WriteDataToGlobal(&effect, sizeof(effect));

	if (!global)
	{
		return E_FAIL;
	}

	STGMEDIUM stg = GetStgMediumForGlobal(global.get());
	RETURN_IF_FAILED(dataObject->SetData(&ftc, &stg, TRUE));

	// The IDataObject instance has taken ownership of stg at this point, so it's responsible for
	// freeing the data.
	global.release();

	return S_OK;
}