// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <gtest/gtest.h>
#include <wil/resource.h>
#include <string>

class RegistryStorageTest : public testing::Test
{
protected:
	void SetUp() override;
	void TearDown() override;

	void ImportRegistryResource(const std::wstring &filename);

	static inline const std::wstring APPLICATION_TEST_KEY = L"Software\\Explorer++Test";

	wil::unique_hkey m_applicationTestKey;
};
