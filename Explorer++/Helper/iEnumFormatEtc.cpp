/******************************************************************
 *
 * Project: Helper
 * File: iEnumFormatEtc.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides an implementation of iEnumFormatEtc.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "iEnumFormatEtc.h"

using namespace std;

class CEnumFormatEtc : public IEnumFORMATETC
{
public:

	CEnumFormatEtc(list<FORMATETC> feList);
	~CEnumFormatEtc();

	HRESULT		__stdcall	QueryInterface(REFIID iid, void **ppvObject);
	ULONG		__stdcall	AddRef(void);
	ULONG		__stdcall	Release(void);

private:

	HRESULT __stdcall Next(ULONG celt,FORMATETC *rgelt,ULONG *pceltFetched);
	HRESULT __stdcall Skip(ULONG celt);
	HRESULT __stdcall Reset(void);
	HRESULT __stdcall Clone(IEnumFORMATETC **ppEnum);

	LONG			m_lRefCount;

	list<FORMATETC>	m_feList;
	int				m_iIndex;
	int				m_iNumFormats;
};

HRESULT CreateEnumFormatEtc(std::list<FORMATETC> feList,IEnumFORMATETC **ppEnumFormatEtc)
{
	*ppEnumFormatEtc = new CEnumFormatEtc(feList);

	return S_OK;
}

CEnumFormatEtc::CEnumFormatEtc(list<FORMATETC> feList)
{
	m_lRefCount = 1;
	m_iIndex = 0;

	for each(auto fe in feList)
	{
		FORMATETC ftc = fe;

		if(fe.ptd)
		{
			ftc.ptd = (DVTARGETDEVICE *)CoTaskMemAlloc(sizeof(DVTARGETDEVICE));

			if(ftc.ptd != NULL)
			{
				*ftc.ptd = *fe.ptd;
			}
		}
		else
		{
			ftc.ptd = NULL;
		}

		m_feList.push_back(ftc);
	}

	m_iNumFormats = static_cast<int>(feList.size());
}

CEnumFormatEtc::~CEnumFormatEtc()
{

}

/* IUnknown interface members. */
HRESULT __stdcall CEnumFormatEtc::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = NULL;

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

ULONG __stdcall CEnumFormatEtc::AddRef(void)
{
	return InterlockedIncrement(&m_lRefCount);
}

ULONG __stdcall CEnumFormatEtc::Release(void)
{
	LONG lCount = InterlockedDecrement(&m_lRefCount);

	if(lCount == 0)
	{
		delete this;
		return 0;
	}

	return lCount;
}


HRESULT __stdcall CEnumFormatEtc::Next(ULONG celt,FORMATETC *rgelt,ULONG *pceltFetched)
{
	if(m_iIndex >= m_iNumFormats)
	{
		if(pceltFetched != NULL)
			*pceltFetched = 0;

		return S_FALSE;
	}

	int i = 0;

	for(auto itr = m_feList.begin();itr != m_feList.end();itr++)
	{
		if(i == m_iIndex)
		{
			memcpy(&rgelt[0],&(*itr),sizeof(FORMATETC));

			if(itr->ptd)
			{
				rgelt->ptd = (DVTARGETDEVICE *)CoTaskMemAlloc(sizeof(DVTARGETDEVICE));

				if(rgelt->ptd == NULL)
					return S_FALSE;

				if(itr->ptd != NULL)
					*rgelt->ptd = *itr->ptd;
				else
					rgelt->ptd=NULL;
			}

			break;
		}

		i++;
	}

	m_iIndex++;

	if(pceltFetched != NULL)
		*pceltFetched = 1;

	return S_OK;
}

HRESULT __stdcall CEnumFormatEtc::Skip(ULONG celt)
{
	m_iIndex += celt;
	return S_OK;
}

HRESULT __stdcall CEnumFormatEtc::Reset(void)
{
	m_iIndex = 0;
	return S_OK;
}

HRESULT __stdcall CEnumFormatEtc::Clone(IEnumFORMATETC **ppEnum)
{
	return E_NOTIMPL;
}