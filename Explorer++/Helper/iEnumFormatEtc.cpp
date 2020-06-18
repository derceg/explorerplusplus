// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include <list>
#include "iEnumFormatEtc.h"


class EnumFormatEtc : public IEnumFORMATETC
{
public:

	EnumFormatEtc(const std::list<FORMATETC> &feList);
	~EnumFormatEtc();

	HRESULT		__stdcall	QueryInterface(REFIID iid, void **ppvObject);
	ULONG		__stdcall	AddRef();
	ULONG		__stdcall	Release();

private:

	HRESULT __stdcall Next(ULONG celt,FORMATETC *rgelt,ULONG *pceltFetched);
	HRESULT __stdcall Skip(ULONG celt);
	HRESULT __stdcall Reset();
	HRESULT __stdcall Clone(IEnumFORMATETC **ppEnum);

	LONG			m_lRefCount;

	std::list<FORMATETC>	m_feList;
	int				m_iIndex;
	int				m_iNumFormats;
};

HRESULT CreateEnumFormatEtc(const std::list<FORMATETC> &feList,IEnumFORMATETC **ppEnumFormatEtc)
{
	*ppEnumFormatEtc = new EnumFormatEtc(feList);

	return S_OK;
}

EnumFormatEtc::EnumFormatEtc(const std::list<FORMATETC> &feList)
{
	m_lRefCount = 1;
	m_iIndex = 0;

	for(const auto &fe : feList)
	{
		FORMATETC ftc = fe;

		if(fe.ptd != nullptr)
		{
			ftc.ptd = reinterpret_cast<DVTARGETDEVICE *>(CoTaskMemAlloc(fe.ptd->tdSize));

			if(ftc.ptd != nullptr)
			{
				memcpy(ftc.ptd, fe.ptd, fe.ptd->tdSize);
			}
		}

		m_feList.push_back(ftc);
	}

	m_iNumFormats = static_cast<int>(feList.size());
}

EnumFormatEtc::~EnumFormatEtc()
{
	for(auto fe : m_feList)
	{
		if(fe.ptd != nullptr)
		{
			CoTaskMemFree(fe.ptd);
		}
	}
}

/* IUnknown interface members. */
HRESULT __stdcall EnumFormatEtc::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = nullptr;

	if(iid == IID_IEnumFORMATETC||iid == IID_IUnknown)
	{
		*ppvObject=this;
	}

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall EnumFormatEtc::AddRef()
{
	return InterlockedIncrement(&m_lRefCount);
}

ULONG __stdcall EnumFormatEtc::Release()
{
	LONG lCount = InterlockedDecrement(&m_lRefCount);

	if(lCount == 0)
	{
		delete this;
		return 0;
	}

	return lCount;
}


HRESULT __stdcall EnumFormatEtc::Next(ULONG celt,FORMATETC *rgelt,ULONG *pceltFetched)
{
	UNREFERENCED_PARAMETER(celt);

	if(m_iIndex >= m_iNumFormats)
	{
		if (pceltFetched != nullptr)
		{
			*pceltFetched = 0;
		}

		return S_FALSE;
	}

	int i = 0;

	for(auto itr = m_feList.begin();itr != m_feList.end();itr++)
	{
		if(i == m_iIndex)
		{
			memcpy(&rgelt[0],&(*itr),sizeof(FORMATETC));

			if(itr->ptd != nullptr)
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

HRESULT __stdcall EnumFormatEtc::Skip(ULONG celt)
{
	m_iIndex += celt;
	return S_OK;
}

HRESULT __stdcall EnumFormatEtc::Reset()
{
	m_iIndex = 0;
	return S_OK;
}

HRESULT __stdcall EnumFormatEtc::Clone(IEnumFORMATETC **ppEnum)
{
	UNREFERENCED_PARAMETER(ppEnum);

	return E_NOTIMPL;
}