// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <list>

HRESULT CreateEnumFormatEtc(const std::list<FORMATETC> &feList, IEnumFORMATETC **ppEnumFormatEtc);