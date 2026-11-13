// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/FileStreamOperations.h"
#include "TemporaryFileTest.h"
#include <wil/resource.h>
#include <winioctl.h>
#include <limits>

using namespace FileStreamOperations;

class FileStreamOperationsTest : public TemporaryFileTest
{
};

class FaultIo : public Io
{
public:
	DWORD readLimit = MAXDWORD;
	DWORD writeLimit = MAXDWORD;
	HRESULT readError = S_OK;
	HRESULT writeError = S_OK;
	HRESULT flushError = S_OK;
	std::stop_source *cancelOnWrite = nullptr;
	std::stop_source *cancelOnFlush = nullptr;
	HRESULT Read(HANDLE file, void *buffer, DWORD size, DWORD &count) override
	{
		if (FAILED(readError))
			return readError;
		return Io::Read(file, buffer, (std::min) (size, readLimit), count);
	}
	HRESULT Write(HANDLE file, const void *buffer, DWORD size, DWORD &count) override
	{
		if (cancelOnWrite)
			cancelOnWrite->request_stop();
		if (FAILED(writeError))
			return writeError;
		return Io::Write(file, buffer, (std::min) (size, writeLimit), count);
	}
	HRESULT Flush(HANDLE file) override
	{
		HRESULT error = FAILED(flushError) ? flushError : Io::Flush(file);
		if (cancelOnFlush)
			cancelOnFlush->request_stop();
		return error;
	}
};

TEST_F(FileStreamOperationsTest, CancellationDuringFlushDoesNotPublish)
{
	Write(L"source", "data");
	std::stop_source stop;
	FaultIo io;
	io.cancelOnFlush = &stop;
	EXPECT_TRUE(Merge({ Path(L"source") }, Path(L"result"), stop.get_token(), {}, &io).Cancelled());
	EXPECT_FALSE(std::filesystem::exists(Path(L"result")));
	stop = std::stop_source();
	EXPECT_TRUE(
		Split(Path(L"source"), m_directory.wstring(), L"part./N", 4, stop.get_token(), {}, &io)
			.Cancelled());
	EXPECT_FALSE(std::filesystem::exists(Path(L"part.1")));
	ExpectNoTemporaryFiles();
}

TEST_F(FileStreamOperationsTest, MergeUsesTheOrderDisplayedAfterMovingAMiddleItem)
{
	Write(L"a", "A");
	Write(L"b", "B");
	Write(L"c", "C");
	Write(L"d", "D");
	std::vector<std::wstring> paths{ Path(L"a"), Path(L"b"), Path(L"c"), Path(L"d") };
	ASSERT_TRUE(MoveItem(paths, 1, false));
	ASSERT_HRESULT_SUCCEEDED(Merge(paths, Path(L"output")).error);
	EXPECT_EQ(Read(L"output"), "ACBD");
	ASSERT_TRUE(MoveItem(paths, 2, true));
	EXPECT_EQ(paths, (std::vector<std::wstring>{ Path(L"a"), Path(L"b"), Path(L"c"), Path(L"d") }));
	EXPECT_FALSE(MoveItem(paths, 0, true));
	EXPECT_FALSE(MoveItem(paths, 3, false));
	EXPECT_FALSE(MoveItem(paths, 4, false));
}

TEST_F(FileStreamOperationsTest, SplitThenMergePreservesBinaryDataAndEmptyInputs)
{
	std::string data(2 * 1024 * 1024 + 37, '\0');
	for (size_t i = 0; i < data.size(); ++i)
		data[i] = static_cast<char>(i % 251);
	Write(L"source", data);
	Write(L"empty", "");
	ASSERT_HRESULT_SUCCEEDED(
		Split(Path(L"source"), m_directory.wstring(), L"part./N", 1024 * 1024).error);
	ASSERT_HRESULT_SUCCEEDED(Merge(
		{ Path(L"part.1"), Path(L"empty"), Path(L"part.2"), Path(L"part.3") }, Path(L"result"))
			.error);
	EXPECT_EQ(Read(L"result"), data);
	EXPECT_EQ(std::filesystem::file_size(Path(L"part.3")), 37u);
	ExpectNoTemporaryFiles();
}

TEST_F(FileStreamOperationsTest, ExistingOutputAndSourceAreNeverOverwritten)
{
	Write(L"source", "source");
	Write(L"output", "keep");
	EXPECT_HRESULT_FAILED(Merge({ Path(L"source") }, Path(L"output")).error);
	EXPECT_EQ(Read(L"output"), "keep");
	EXPECT_HRESULT_FAILED(Merge({ Path(L"source") }, Path(L"source")).error);
	EXPECT_EQ(Read(L"source"), "source");
	ExpectNoTemporaryFiles();
}

TEST_F(FileStreamOperationsTest, MissingInputFailsTheWholeMerge)
{
	Write(L"first", "first");
	auto result = Merge({ Path(L"first"), Path(L"missing") }, Path(L"output"));
	EXPECT_EQ(result.error, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
	EXPECT_EQ(result.path, Path(L"missing"));
	EXPECT_FALSE(std::filesystem::exists(Path(L"output")));
	ExpectNoTemporaryFiles();
}

TEST_F(FileStreamOperationsTest, SplitCollisionRollsBackPublishedPartsAndKeepsExistingFile)
{
	Write(L"source", "0123456789");
	Write(L"part.2", "keep");
	EXPECT_HRESULT_FAILED(Split(Path(L"source"), m_directory.wstring(), L"part./N", 4).error);
	EXPECT_FALSE(std::filesystem::exists(Path(L"part.1")));
	EXPECT_FALSE(std::filesystem::exists(Path(L"part.3")));
	EXPECT_EQ(Read(L"part.2"), "keep");
	EXPECT_EQ(Read(L"source"), "0123456789");
	ExpectNoTemporaryFiles();
}

TEST_F(FileStreamOperationsTest, ShortReadsAndWritesAreCompleted)
{
	Write(L"source", std::string(8193, 'x'));
	FaultIo io;
	io.readLimit = 17;
	io.writeLimit = 3;
	ASSERT_HRESULT_SUCCEEDED(Merge({ Path(L"source") }, Path(L"result"), {}, {}, &io).error);
	EXPECT_EQ(Read(L"result"), Read(L"source"));
}

TEST_F(FileStreamOperationsTest, ReadWriteAndFlushFailuresDoNotPublishOutputs)
{
	Write(L"source", std::string(8193, 'x'));
	for (int failure = 0; failure < 5; ++failure)
	{
		SCOPED_TRACE(failure);
		FaultIo io;
		if (failure == 0)
			io.readError = HRESULT_FROM_WIN32(ERROR_CRC);
		if (failure == 1)
			io.writeError = HRESULT_FROM_WIN32(ERROR_DISK_FULL);
		if (failure == 2)
			io.flushError = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
		if (failure == 3)
			io.readLimit = 0;
		if (failure == 4)
			io.writeLimit = 0;
		EXPECT_HRESULT_FAILED(Merge({ Path(L"source") }, Path(L"result"), {}, {}, &io).error);
		EXPECT_HRESULT_FAILED(
			Split(Path(L"source"), m_directory.wstring(), L"part./N", 4096, {}, {}, &io).error);
		EXPECT_FALSE(std::filesystem::exists(Path(L"result")));
		EXPECT_FALSE(std::filesystem::exists(Path(L"part.1")));
		ExpectNoTemporaryFiles();
	}
}

TEST_F(FileStreamOperationsTest, CancellationInsideOneLargePartRemovesAllOwnedData)
{
	Write(L"source", std::string(2 * 1024 * 1024, 'x'));
	std::stop_source stop;
	FaultIo io;
	io.cancelOnWrite = &stop;
	EXPECT_TRUE(Split(Path(L"source"), m_directory.wstring(), L"part./N", 4ULL * 1024 * 1024,
		stop.get_token(), {}, &io)
			.Cancelled());
	EXPECT_FALSE(std::filesystem::exists(Path(L"part.1")));
	stop = std::stop_source();
	EXPECT_TRUE(Merge({ Path(L"source") }, Path(L"result"), stop.get_token(), {}, &io).Cancelled());
	EXPECT_FALSE(std::filesystem::exists(Path(L"result")));
	ExpectNoTemporaryFiles();
}

TEST_F(FileStreamOperationsTest, CancelBetweenPartsRollsBackAllStagedParts)
{
	Write(L"source", "0123456789");
	std::stop_source stop;
	auto result = Split(Path(L"source"), m_directory.wstring(), L"part./N", 4, stop.get_token(),
		[&stop](uint64_t, uint64_t) { stop.request_stop(); });
	EXPECT_TRUE(result.Cancelled());
	EXPECT_FALSE(std::filesystem::exists(Path(L"part.1")));
	ExpectNoTemporaryFiles();
}

TEST_F(FileStreamOperationsTest, InvalidSizeAndPatternFailWithoutCreatingFiles)
{
	Write(L"source", "abc");
	EXPECT_EQ(Split(Path(L"source"), m_directory.wstring(), L"part./N", 0).error, E_INVALIDARG);
	EXPECT_EQ(Split(Path(L"source"), m_directory.wstring(), L"part", 1).error, E_INVALIDARG);
	EXPECT_EQ(Split(Path(L"source"), m_directory.wstring(), L"../part./N", 1).error, E_INVALIDARG);
	EXPECT_FALSE(ParsePartSize(L"0", 1));
	EXPECT_FALSE(ParsePartSize(L"-1", 1));
	EXPECT_FALSE(ParsePartSize(L"1x", 1));
	EXPECT_FALSE(ParsePartSize(L"18446744073709551616", 1));
	EXPECT_FALSE(ParsePartSize(L"8589934592", 1024ULL * 1024 * 1024));
	EXPECT_EQ(ParsePartSize(L"4", 1024ULL * 1024 * 1024), 4294967296ULL);
	EXPECT_EQ(ParsePartSize(L"4294967296", 1), 4294967296ULL);
	ExpectNoTemporaryFiles();
}

TEST_F(FileStreamOperationsTest, LargeFileRoundTripAcrossFourGiBBoundary)
{
	constexpr uint64_t boundary = 4ULL * 1024 * 1024 * 1024;
	constexpr uint64_t size = boundary + 37;
	if (std::filesystem::space(m_directory).available < boundary + 256 * 1024 * 1024)
	{
		GTEST_SKIP() << "The large-file regression needs 4.25 GiB of free space";
	}
	wil::unique_hfile source(CreateFile(Path(L"source").c_str(), GENERIC_READ | GENERIC_WRITE, 0,
		nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
	ASSERT_TRUE(source);
	DWORD count;
	ASSERT_TRUE(
		DeviceIoControl(source.get(), FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &count, nullptr));
	LARGE_INTEGER position;
	position.QuadPart = size;
	ASSERT_TRUE(SetFilePointerEx(source.get(), position, nullptr, FILE_BEGIN));
	ASSERT_TRUE(SetEndOfFile(source.get()));
	const std::string marker(4096, 'X');
	for (uint64_t offset : { 0ULL, boundary - 4096, size - marker.size() })
	{
		position.QuadPart = offset;
		ASSERT_TRUE(SetFilePointerEx(source.get(), position, nullptr, FILE_BEGIN));
		ASSERT_TRUE(WriteFile(source.get(), marker.data(), static_cast<DWORD>(marker.size()),
			&count, nullptr));
	}
	source.reset();
	ASSERT_HRESULT_SUCCEEDED(
		Split(Path(L"source"), m_directory.wstring(), L"part./N", boundary).error);
	EXPECT_EQ(std::filesystem::file_size(Path(L"part.1")), boundary);
	EXPECT_EQ(std::filesystem::file_size(Path(L"part.2")), 37u);
	auto matches = [&](const wchar_t *actualName, uint64_t length, uint64_t sourceOffset)
	{
		std::ifstream expected(std::filesystem::path(Path(L"source")), std::ios::binary);
		std::ifstream actual(std::filesystem::path(Path(actualName)), std::ios::binary);
		expected.seekg(static_cast<std::streamoff>(sourceOffset));
		std::vector<char> first(1024 * 1024), second(first.size());
		while (length != 0)
		{
			auto count = static_cast<std::streamsize>(
				(std::min) (length, static_cast<uint64_t>(first.size())));
			expected.read(first.data(), count);
			actual.read(second.data(), count);
			if (expected.gcount() != count || actual.gcount() != count
				|| !std::equal(first.begin(), first.begin() + static_cast<std::ptrdiff_t>(count),
					second.begin()))
				return false;
			length -= count;
		}
		return true;
	};
	ASSERT_TRUE(matches(L"part.1", boundary, 0));
	ASSERT_TRUE(matches(L"part.2", 37, boundary));

	// Release only the zero range already verified above. This keeps peak disk
	// usage near 4 GiB instead of 8 GiB while both transfers use real WriteFile.
	wil::unique_hfile firstPart(CreateFile(Path(L"part.1").c_str(), GENERIC_WRITE, 0, nullptr,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
	ASSERT_TRUE(firstPart);
	ASSERT_TRUE(DeviceIoControl(firstPart.get(), FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &count,
		nullptr));
	FILE_ZERO_DATA_INFORMATION zeros{};
	zeros.FileOffset.QuadPart = 4096;
	zeros.BeyondFinalZero.QuadPart = boundary - 4096;
	ASSERT_TRUE(DeviceIoControl(firstPart.get(), FSCTL_SET_ZERO_DATA, &zeros, sizeof(zeros),
		nullptr, 0, &count, nullptr));
	firstPart.reset();
	ASSERT_HRESULT_SUCCEEDED(Merge({ Path(L"part.1"), Path(L"part.2") }, Path(L"result")).error);
	ASSERT_EQ(std::filesystem::file_size(Path(L"result")), size);
	ASSERT_TRUE(matches(L"result", size, 0));
	ExpectNoTemporaryFiles();
}
