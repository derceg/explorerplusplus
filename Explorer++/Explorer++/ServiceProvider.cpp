// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ServiceProvider.h"

// IServiceProvider
IFACEMETHODIMP ServiceProvider::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
	return ServiceProviderBase::QueryService(guidService, riid, ppv);
}
