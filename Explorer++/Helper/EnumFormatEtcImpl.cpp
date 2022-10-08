// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "EnumFormatEtcImpl.h"

EnumFormatEtcImpl::EnumFormatEtcImpl(const std::list<FORMATETC> &feList)
{
	m_iIndex = 0;

	for (const auto &fe : feList)
	{
		FORMATETC ftc = fe;

		if (fe.ptd != nullptr)
		{
			ftc.ptd = reinterpret_cast<DVTARGETDEVICE *>(CoTaskMemAlloc(fe.ptd->tdSize));

			if (ftc.ptd != nullptr)
			{
				memcpy(ftc.ptd, fe.ptd, fe.ptd->tdSize);
			}
		}

		m_feList.push_back(ftc);
	}

	m_iNumFormats = static_cast<int>(feList.size());
}

EnumFormatEtcImpl::~EnumFormatEtcImpl()
{
	for (auto fe : m_feList)
	{
		if (fe.ptd != nullptr)
		{
			CoTaskMemFree(fe.ptd);
		}
	}
}

// IEnumFORMATETC
IFACEMETHODIMP EnumFormatEtcImpl::Next(ULONG celt, FORMATETC *rgelt, ULONG *pceltFetched)
{
	UNREFERENCED_PARAMETER(celt);

	if (m_iIndex >= m_iNumFormats)
	{
		if (pceltFetched != nullptr)
		{
			*pceltFetched = 0;
		}

		return S_FALSE;
	}

	int i = 0;

	for (auto itr = m_feList.begin(); itr != m_feList.end(); itr++)
	{
		if (i == m_iIndex)
		{
			memcpy(&rgelt[0], &(*itr), sizeof(FORMATETC));

			if (itr->ptd != nullptr)
			{
				rgelt[0].ptd = reinterpret_cast<DVTARGETDEVICE *>(CoTaskMemAlloc(itr->ptd->tdSize));

				if (rgelt[0].ptd == nullptr)
				{
					return S_FALSE;
				}

				memcpy(rgelt[0].ptd, itr->ptd, itr->ptd->tdSize);
			}

			break;
		}

		i++;
	}

	m_iIndex++;

	if (pceltFetched != nullptr)
	{
		*pceltFetched = 1;
	}

	return S_OK;
}

IFACEMETHODIMP EnumFormatEtcImpl::Skip(ULONG celt)
{
	m_iIndex += celt;
	return S_OK;
}

IFACEMETHODIMP EnumFormatEtcImpl::Reset()
{
	m_iIndex = 0;
	return S_OK;
}

IFACEMETHODIMP EnumFormatEtcImpl::Clone(IEnumFORMATETC **ppEnum)
{
	UNREFERENCED_PARAMETER(ppEnum);

	return E_NOTIMPL;
}
