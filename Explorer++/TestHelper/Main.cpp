#include "stdafx.h"
#include "gtest\gtest.h"

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