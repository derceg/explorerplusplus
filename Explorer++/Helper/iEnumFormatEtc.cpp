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
#include <string>

class CEnumFormatEtc : public IEnumFORMATETC
{
public:
	CEnumFormatEtc(FORMATETC *,int);
	~CEnumFormatEtc();

	HRESULT		__stdcall	QueryInterface(REFIID iid, void **ppvObject);
	ULONG		__stdcall	AddRef(void);
	ULONG		__stdcall	Release(void);

private:
	HRESULT __stdcall Next(ULONG celt,FORMATETC *rgelt,ULONG *pceltFetched);
	HRESULT __stdcall Skip(ULONG celt);
	HRESULT __stdcall Reset(void);
	HRESULT __stdcall Clone(IEnumFORMATETC **ppEnum);

	FORMATETC *m_pFormatEtc;
	int m_iRefCount;
	int m_iIndex;
	int m_iNumFormats;
};

HRESULT CreateEnumFormatEtc(FORMATETC *pFormatEtc,IEnumFORMATETC **ppEnumFormatEtc,int count)
{
	*ppEnumFormatEtc = new CEnumFormatEtc(pFormatEtc,count);

	return S_OK;
}

CEnumFormatEtc::CEnumFormatEtc(FORMATETC *pFormatEtc,int count)
{
	m_iRefCount		= 1;

	m_iIndex		= 0;
	m_iNumFormats	= 0;
	m_pFormatEtc	= new FORMATETC[count];

	int i =0;

	for(i =0;i <= (count - 1);i++)
	{
		memcpy(&m_pFormatEtc[i],&pFormatEtc[i],sizeof(FORMATETC));

		if(pFormatEtc[i].ptd)
		{
			m_pFormatEtc[i].ptd=(DVTARGETDEVICE *)CoTaskMemAlloc(sizeof(DVTARGETDEVICE));

			if(m_pFormatEtc[i].ptd != NULL)
			{
				*m_pFormatEtc[i].ptd = *pFormatEtc[i].ptd;
			}
		}
		else
		{
			m_pFormatEtc[i].ptd = NULL;
		}

		m_iNumFormats++;
	}
}

CEnumFormatEtc::~CEnumFormatEtc()
{

}

/*IUnknown interface members.*/
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
	return ++m_iRefCount;
}

ULONG __stdcall CEnumFormatEtc::Release(void)
{
	m_iRefCount--;

	if(m_iRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_iRefCount;
}


HRESULT __stdcall CEnumFormatEtc::Next(ULONG celt,FORMATETC *rgelt,ULONG *pceltFetched)
{
	if(m_iIndex >= m_iNumFormats)
	{
		return S_FALSE;
	}

	memcpy(&rgelt[0],&m_pFormatEtc[m_iIndex],sizeof(FORMATETC));

	if(m_pFormatEtc[m_iIndex].ptd)
	{
		rgelt->ptd = (DVTARGETDEVICE *)CoTaskMemAlloc(sizeof(DVTARGETDEVICE));

		if(rgelt->ptd == NULL)
			return S_FALSE;

		if(m_pFormatEtc[m_iIndex].ptd != NULL)
			*rgelt->ptd = *m_pFormatEtc[m_iIndex].ptd;
		else
			rgelt->ptd=NULL;
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
	return S_OK;
}