// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "WinRTBaseWrapper.h"
#include <list>

class EnumFormatEtcImpl : public winrt::implements<EnumFormatEtcImpl, IEnumFORMATETC>
{
public:
	EnumFormatEtcImpl(const std::list<FORMATETC> &feList);
	~EnumFormatEtcImpl();

private:
	// IEnumFORMATETC
	IFACEMETHODIMP Next(ULONG celt, FORMATETC *rgelt, ULONG *pceltFetched);
	IFACEMETHODIMP Skip(ULONG celt);
	IFACEMETHODIMP Reset();
	IFACEMETHODIMP Clone(IEnumFORMATETC **ppEnum);

	std::list<FORMATETC> m_feList;
	int m_iIndex;
	int m_iNumFormats;
};
