// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ServiceProvider.h"

wil::com_ptr_nothrow<ServiceProvider> ServiceProvider::Create()
{
	wil::com_ptr_nothrow<ServiceProvider> serviceProvider;
	serviceProvider.attach(new ServiceProvider());
	return serviceProvider;
}

ServiceProvider::ServiceProvider() : m_refCount(1)
{
}

void ServiceProvider::RegisterService(REFGUID guidService, IUnknown *service)
{
	m_services[guidService] = service;
}

// IServiceProvider
IFACEMETHODIMP ServiceProvider::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
	UNREFERENCED_PARAMETER(guidService);

	auto itr = m_services.find(guidService);

	if (itr == m_services.end())
	{
		return E_NOINTERFACE;
	}

	return itr->second->QueryInterface(riid, ppv);
}

// IUnknown
IFACEMETHODIMP ServiceProvider::QueryInterface(REFIID riid, void **ppvObject)
{
	// clang-format off
	static const QITAB qit[] = {
		QITABENT(ServiceProvider, IServiceProvider),
		{ nullptr }
	};
	// clang-format on

	return QISearch(this, qit, riid, ppvObject);
}

IFACEMETHODIMP_(ULONG) ServiceProvider::AddRef()
{
	return InterlockedIncrement(&m_refCount);
}

IFACEMETHODIMP_(ULONG) ServiceProvider::Release()
{
	ULONG refCount = InterlockedDecrement(&m_refCount);

	if (refCount == 0)
	{
		delete this;
		return 0;
	}

	return refCount;
}