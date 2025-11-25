// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/PidlHelper.h"
#include "PidlTestHelper.h"
#include "../Helper/Pidl.h"
#include <gtest/gtest.h>

using namespace testing;

TEST(PidlBase64Decoding, EmptyData)
{
	auto decodedPidl = DecodePidlFromBase64("");
	ASSERT_FALSE(decodedPidl.HasValue());
}

TEST(PidlBase64Decoding, InvalidData)
{
	// ';' isn't a valid character in the base64 alphabet.
	auto decodedPidl = DecodePidlFromBase64(";");
	ASSERT_FALSE(decodedPidl.HasValue());

	// The encoded data here isn't valid, since it's not long enough to represent a single byte.
	decodedPidl = DecodePidlFromBase64("a");
	ASSERT_FALSE(decodedPidl.HasValue());
}

class PidlBase64Encoding : public Test
{
protected:
	void PerformEncodeDecodeTest(const std::wstring &path)
	{
		auto pidl = CreateSimplePidlForTest(path);
		auto encodedPidl = EncodePidlToBase64(pidl);
		auto decodedPidl = DecodePidlFromBase64(encodedPidl);
		ASSERT_TRUE(decodedPidl.HasValue());
		EXPECT_EQ(decodedPidl, pidl);
	}
};

TEST_F(PidlBase64Encoding, EncodeDecode)
{
	PerformEncodeDecodeTest(LR"(c:\path)");
	PerformEncodeDecodeTest(LR"(c:\windows\system32)");
	PerformEncodeDecodeTest(LR"(d:\path\to\folder)");
}
