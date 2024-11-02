// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellHelper.h"
#include <boost/container_hash/hash.hpp>
#include <wil/com.h>
#include <unordered_map>

class ServiceProviderBase
{
public:
	void RegisterService(REFGUID guidService, IUnknown *service);
	HRESULT QueryService(REFGUID guidService, REFIID riid, void **ppv);

protected:
	ServiceProviderBase() = default;

private:
	std::unordered_map<IID, wil::com_ptr_nothrow<IUnknown>, boost::hash<IID>> m_services;
};
