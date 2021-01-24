// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ShellHelper.h"
#include <boost/container_hash/hash.hpp>
#include <wil/com.h>
#include <unordered_map>

class ServiceProvider : public IServiceProvider
{
public:
	static wil::com_ptr_nothrow<ServiceProvider> Create();

	void RegisterService(REFGUID guidService, IUnknown *service);

	// IServiceProvider
	IFACEMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);

	// IUnknown
	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
	IFACEMETHODIMP_(ULONG) AddRef();
	IFACEMETHODIMP_(ULONG) Release();

private:
	ServiceProvider();

	ULONG m_refCount;
	std::unordered_map<IID, wil::com_ptr_nothrow<IUnknown>, boost::hash<IID>> m_services;
};