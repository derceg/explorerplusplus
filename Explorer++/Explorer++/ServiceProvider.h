// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ServiceProviderBase.h"
#include <servprov.h>

class ServiceProvider :
	public winrt::implements<ServiceProvider, IServiceProvider>,
	public ServiceProviderBase
{
public:
	// IServiceProvider
	IFACEMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);
};