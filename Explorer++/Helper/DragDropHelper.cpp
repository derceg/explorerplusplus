// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DragDropHelper.h"

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