// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <gtest/gtest.h>
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <string>

class TemporaryFileTest : public testing::Test
{
protected:
	void SetUp() override
	{
		GUID id;
		ASSERT_HRESULT_SUCCEEDED(CoCreateGuid(&id));
		wchar_t name[40];
		StringFromGUID2(id, name, static_cast<int>(std::size(name)));
		m_directory = std::filesystem::temp_directory_path() / name;
		ASSERT_TRUE(std::filesystem::create_directory(m_directory));
	}
	void TearDown() override
	{
		std::error_code error;
		std::filesystem::remove_all(m_directory, error);
		EXPECT_FALSE(error) << error.message();
	}
	std::wstring Path(const wchar_t *name) const
	{
		return (m_directory / name).wstring();
	}
	void Write(const wchar_t *name, const std::string &data)
	{
		std::ofstream stream(std::filesystem::path(Path(name)), std::ios::binary);
		stream.write(data.data(), static_cast<std::streamsize>(data.size()));
		ASSERT_TRUE(stream.good());
	}
	std::string Read(const wchar_t *name) const
	{
		std::ifstream stream(std::filesystem::path(Path(name)), std::ios::binary);
		return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
	}
	void ExpectNoTemporaryFiles() const
	{
		for (const auto &entry : std::filesystem::directory_iterator(m_directory))
			EXPECT_EQ(entry.path().wstring().find(L".tmp-"), std::wstring::npos);
	}
	std::filesystem::path m_directory;
};
