// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <shlobj.h>

HRESULT	CreateDataObject(FORMATETC *,STGMEDIUM *,IDataObject **,int);