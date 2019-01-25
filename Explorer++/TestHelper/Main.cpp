// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"

class OleEnvironment : public ::testing::Environment
{
public:

	void SetUp()
	{
		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		ASSERT_TRUE(SUCCEEDED(hr));
	}

	void TearDown()
	{
		CoUninitialize();
	}
};

int _tmain(int argc, _TCHAR* argv[])
{
	::testing::AddGlobalTestEnvironment(new OleEnvironment());
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}