// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <shtypes.h>

STGMEDIUM GetStgMediumForGlobal(HGLOBAL global);
STGMEDIUM GetStgMediumForStream(IStream *stream);
HRESULT SetPreferredDropEffect(IDataObject *dataObject, DWORD effect);
HRESULT CreateDataObjectForShellTransfer(
	const std::vector<PCIDLIST_ABSOLUTE> &items, IDataObject **dataObjectOut);