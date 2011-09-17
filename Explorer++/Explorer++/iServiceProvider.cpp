/******************************************************************
 *
 * Project: Explorer++
 * File: iServiceProvider.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Part of the internal system that supports the new menu.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "iServiceProvider.h"
#include "iShellView2.h"
#include "INewMenuClient.h"


CServiceProvider::CServiceProvider(IExplorerplusplus *pexpp) :
m_pexpp(pexpp),
m_RefCount(1)
{

}

CServiceProvider::~CServiceProvider()
{

}

HRESULT __stdcall CServiceProvider::QueryInterface(REFIID iid,void **ppvObject)
{
	*ppvObject = NULL;

	if(iid == IID_IUnknown)
	{
		*ppvObject = static_cast<IUnknown *>(this);
	}
	else if(iid == IID_IServiceProvider)
	{
		*ppvObject = static_cast<IServiceProvider *>(this);
	}

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall CServiceProvider::AddRef(void)
{
	return ++m_RefCount;
}

ULONG __stdcall CServiceProvider::Release(void)
{
	m_RefCount--;
	
	if(m_RefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_RefCount;
}

HRESULT CServiceProvider::QueryService(REFGUID guidService,REFIID riid,void **ppv)
{
	*ppv = NULL;

	if(riid == IID_IShellView2)
	{
		*ppv = new CShellView2(m_pexpp);
	}
	else if(riid == IID_INewMenuClient)
	{
		*ppv = new CNewMenuClient(m_pexpp);
	}

	if(*ppv)
	{
		return S_OK;
	}

	return E_NOINTERFACE;
}