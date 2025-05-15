// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellHelper.h"
#include "WinRTBaseWrapper.h"
#include <boost/container_hash/hash.hpp>
#include <unordered_map>

class ServiceProviderBase
{
public:
	void RegisterService(REFGUID guidService, winrt::com_ptr<IUnknown> service);
	HRESULT QueryService(REFGUID guidService, REFIID riid, void **ppv);

protected:
	ServiceProviderBase() = default;

private:
	std::unordered_map<IID, winrt::com_ptr<IUnknown>, boost::hash<IID>> m_services;
};
