// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ServiceProvider.h"
#include "../Helper/WinRTBaseWrapper.h"
#include <gtest/gtest.h>

namespace
{

class FakeService : public winrt::implements<FakeService, IUnknown, winrt::non_agile>
{
};

}

TEST(ServiceProvider, NoService)
{
	auto serviceProvider = winrt::make_self<ServiceProvider>();

	wil::com_ptr_nothrow<IUnknown> result;
	EXPECT_EQ(serviceProvider->QueryService(IID_IUnknown, IID_PPV_ARGS(&result)), E_NOINTERFACE);
	EXPECT_EQ(result, nullptr);
}

TEST(ServiceProvider, RegisterService)
{
	auto serviceProvider = winrt::make_self<ServiceProvider>();

	auto fakeService = winrt::make<FakeService>();
	serviceProvider->RegisterService(IID_IUnknown, fakeService.get());

	wil::com_ptr_nothrow<IUnknown> result;
	EXPECT_HRESULT_SUCCEEDED(serviceProvider->QueryService(IID_IUnknown, IID_PPV_ARGS(&result)));
	EXPECT_EQ(result, fakeService.get());
}
