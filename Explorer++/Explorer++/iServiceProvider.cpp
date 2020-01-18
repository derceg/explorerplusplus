// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Part of the internal system that supports the new menu.
 */

#include "stdafx.h"
#include "iServiceProvider.h"
#include "INewMenuClient.h"


ServiceProvider::ServiceProvider(IExplorerplusplus *pexpp) :
m_pexpp(pexpp),
m_RefCount(1)
{

}

HRESULT __stdcall ServiceProvider::QueryInterface(REFIID iid,void **ppvObject)
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

ULONG __stdcall ServiceProvider::AddRef(void)
{
	return ++m_RefCount;
}

ULONG __stdcall ServiceProvider::Release(void)
{
	m_RefCount--;
	
	if(m_RefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_RefCount;
}

HRESULT ServiceProvider::QueryService(REFGUID guidService,REFIID riid,void **ppv)
{
	/* Technically, this parameter should
	probably be validated, but in this case
	it likely doesn't matter.
	There aren't separate services, and this
	class is used for one specific purpose
	(providing an interface for the "new"
	menu). */
	UNREFERENCED_PARAMETER(guidService);

	*ppv = NULL;

	if(riid == IID_INewMenuClient)
	{
		*ppv = new NewMenuClient(m_pexpp);
	}

	if(*ppv)
	{
		return S_OK;
	}

	return E_NOINTERFACE;
}