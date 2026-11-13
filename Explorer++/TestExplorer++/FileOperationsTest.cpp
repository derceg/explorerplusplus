// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/FileOperations.h"
#include "TemporaryFileTest.h"
#include <wil/resource.h>

class FileOperationsTest : public TemporaryFileTest
{
};

namespace
{
class OperationStub : public IFileOperation
{
public:
	HRESULT performResult = S_OK;
	HRESULT queryResult = S_OK;
	BOOL aborted = FALSE;
	bool queried = false;
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void **object) override
	{
		*object = nullptr;
		return E_NOINTERFACE;
	}
	ULONG STDMETHODCALLTYPE AddRef() override
	{
		return 1;
	}
	ULONG STDMETHODCALLTYPE Release() override
	{
		return 1;
	}
	HRESULT STDMETHODCALLTYPE Advise(IFileOperationProgressSink *, DWORD *) override
	{
		return E_NOTIMPL;
	}
	HRESULT STDMETHODCALLTYPE Unadvise(DWORD) override
	{
		return E_NOTIMPL;
	}
	HRESULT STDMETHODCALLTYPE SetOperationFlags(DWORD) override
	{
		return E_NOTIMPL;
	}
	HRESULT STDMETHODCALLTYPE SetProgressMessage(LPCWSTR) override
	{
		return E_NOTIMPL;
	}
	HRESULT STDMETHODCALLTYPE SetProgressDialog(IOperationsProgressDialog *) override
	{
		return E_NOTIMPL;
	}
	HRESULT STDMETHODCALLTYPE SetProperties(IPropertyChangeArray *) override
	{
		return E_NOTIMPL;
	}
	HRESULT STDMETHODCALLTYPE SetOwnerWindow(HWND) override
	{
		return E_NOTIMPL;
	}
	HRESULT STDMETHODCALLTYPE ApplyPropertiesToItem(IShellItem *) override
	{
		return E_NOTIMPL;
	}
	HRESULT STDMETHODCALLTYPE ApplyPropertiesToItems(IUnknown *) override
	{
		return E_NOTIMPL;
	}
	HRESULT STDMETHODCALLTYPE RenameItem(IShellItem *, LPCWSTR,
		IFileOperationProgressSink *) override
	{
		return E_NOTIMPL;
	}
	HRESULT STDMETHODCALLTYPE RenameItems(IUnknown *, LPCWSTR) override
	{
		return E_NOTIMPL;
	}
	HRESULT STDMETHODCALLTYPE MoveItem(IShellItem *, IShellItem *, LPCWSTR,
		IFileOperationProgressSink *) override
	{
		return E_NOTIMPL;
	}
	HRESULT STDMETHODCALLTYPE MoveItems(IUnknown *, IShellItem *) override
	{
		return E_NOTIMPL;
	}
	HRESULT STDMETHODCALLTYPE CopyItem(IShellItem *, IShellItem *, LPCWSTR,
		IFileOperationProgressSink *) override
	{
		return E_NOTIMPL;
	}
	HRESULT STDMETHODCALLTYPE CopyItems(IUnknown *, IShellItem *) override
	{
		return E_NOTIMPL;
	}
	HRESULT STDMETHODCALLTYPE DeleteItem(IShellItem *, IFileOperationProgressSink *) override
	{
		return E_NOTIMPL;
	}
	HRESULT STDMETHODCALLTYPE DeleteItems(IUnknown *) override
	{
		return E_NOTIMPL;
	}
	HRESULT STDMETHODCALLTYPE NewItem(IShellItem *, DWORD, LPCWSTR, LPCWSTR,
		IFileOperationProgressSink *) override
	{
		return E_NOTIMPL;
	}
	HRESULT STDMETHODCALLTYPE PerformOperations() override
	{
		return performResult;
	}
	HRESULT STDMETHODCALLTYPE GetAnyOperationsAborted(BOOL *value) override
	{
		queried = true;
		*value = aborted;
		return queryResult;
	}
};
}

TEST(ShellOperationResult, CancellationIsNotReportedAsSuccess)
{
	OperationStub operation;
	operation.aborted = TRUE;
	EXPECT_EQ(FileOperations::PerformOperations(&operation), HRESULT_FROM_WIN32(ERROR_CANCELLED));
	EXPECT_TRUE(operation.queried);
}

TEST(ShellOperationResult, ExecutionErrorIsPreservedWithoutQueryingCompletion)
{
	OperationStub operation;
	operation.performResult = E_ACCESSDENIED;
	EXPECT_EQ(FileOperations::PerformOperations(&operation), E_ACCESSDENIED);
	EXPECT_FALSE(operation.queried);
}

TEST(ShellOperationResult, CompletionQueryErrorIsPreserved)
{
	OperationStub operation;
	operation.queryResult = E_FAIL;
	EXPECT_EQ(FileOperations::PerformOperations(&operation), E_FAIL);
}

TEST(ShellOperationResult, CompletedBatchSucceeds)
{
	OperationStub operation;
	EXPECT_HRESULT_SUCCEEDED(FileOperations::PerformOperations(&operation));
	EXPECT_TRUE(operation.queried);
}

TEST_F(FileOperationsTest, DirectoryListingWritesUtf16AndReleasesTheOutputHandle)
{
	auto source = m_directory / L"directory";
	ASSERT_TRUE(std::filesystem::create_directory(source));
	std::ofstream(source / L"entry.txt") << "data";
	ASSERT_HRESULT_SUCCEEDED(
		FileOperations::SaveDirectoryListing(source.wstring(), Path(L"listing")));
	auto data = Read(L"listing");
	ASSERT_GE(data.size(), 2u);
	EXPECT_EQ(static_cast<unsigned char>(data[0]), 0xff);
	EXPECT_EQ(static_cast<unsigned char>(data[1]), 0xfe);
	std::wstring text((data.size() - 2) / sizeof(wchar_t), L'\0');
	memcpy(text.data(), data.data() + 2, text.size() * sizeof(wchar_t));
	EXPECT_NE(text.find(L"entry.txt"), std::wstring::npos);
	EXPECT_TRUE(DeleteFile(Path(L"listing").c_str()));
	ExpectNoTemporaryFiles();
}

TEST_F(FileOperationsTest, DirectoryListingDoesNotTruncateALockedDestination)
{
	Write(L"listing", "keep");
	wil::unique_hfile lock(CreateFile(Path(L"listing").c_str(), GENERIC_READ, FILE_SHARE_READ,
		nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
	ASSERT_TRUE(lock);
	EXPECT_HRESULT_FAILED(
		FileOperations::SaveDirectoryListing(m_directory.wstring(), Path(L"listing")));
	EXPECT_EQ(Read(L"listing"), "keep");
	ExpectNoTemporaryFiles();
}

TEST_F(FileOperationsTest, DirectoryListingReportsSourceEnumerationFailure)
{
	EXPECT_HRESULT_FAILED(FileOperations::SaveDirectoryListing(Path(L"missing"), Path(L"listing")));
	EXPECT_FALSE(std::filesystem::exists(Path(L"listing")));
	ExpectNoTemporaryFiles();
}
