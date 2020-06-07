// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "Plugins/Manifest.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

TEST(ManifestTest, TestParsing)
{
	// clang-format off
	nlohmann::json json = {
		{"name", "Test plugin"},
		{"description", "Test description"},
		{"file", "plugin.lua"},
		{"version", "1.0"},
		{"author", "John Smith"}
	};
	// clang-format on

	Plugins::Manifest manifest = json.get<Plugins::Manifest>();

	EXPECT_EQ(manifest.name, L"Test plugin");
	EXPECT_EQ(manifest.description, L"Test description");
	EXPECT_EQ(manifest.file, L"plugin.lua");
	EXPECT_EQ(manifest.version, L"1.0");
	EXPECT_EQ(manifest.author, L"John Smith");
}