// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "../Helper/FolderSize.h"
#include "../Helper/Macros.h"
#include "Helper.h"

void TestCalculateFolderSize(const TCHAR *szFolder, int nFoldersExpected,
	int nFilesExpected, ULARGE_INTEGER ulTotalFolderSizeExpected)
{
	TCHAR szFullFileName[MAX_PATH];
	GetTestResourceFilePath(szFolder, szFullFileName, SIZEOF_ARRAY(szFullFileName));

	int nFolders;
	int nFiles;
	ULARGE_INTEGER ulTotalFolderSize;
	HRESULT hr = CalculateFolderSize(szFullFileName, &nFolders, &nFiles, &ulTotalFolderSize);
	ASSERT_TRUE(SUCCEEDED(hr));

	EXPECT_EQ(nFoldersExpected, nFolders);
	EXPECT_EQ(nFilesExpected, nFiles);
	EXPECT_EQ(ulTotalFolderSizeExpected.QuadPart, ulTotalFolderSize.QuadPart);
}

class CalculateFolderSizeTest : public ::testing::Test
{
public:

	CalculateFolderSizeTest() : m_bDeleteDirectory(FALSE)
	{

	}

protected:

	void SetUp()
	{
		/* Git doesn't allow truly empty
		folders to be stored within a
		repository, so create an empty
		folder, run the test, then delete
		it. */
		GetTestResourceDirectory(m_szEmptyDirectoryPath, SIZEOF_ARRAY(m_szEmptyDirectoryPath));
		BOOL bRet = PathAppend(m_szEmptyDirectoryPath, EMPTY_DIRECTORY_NAME);
		ASSERT_EQ(TRUE, bRet);

		bRet = CreateDirectory(m_szEmptyDirectoryPath, NULL);
		ASSERT_EQ(TRUE, bRet);

		m_bDeleteDirectory = TRUE;
	}

	void TearDown()
	{
		if(m_bDeleteDirectory)
		{
			BOOL bRet = RemoveDirectory(m_szEmptyDirectoryPath);
			ASSERT_EQ(TRUE, bRet);
		}
	}

	static const TCHAR EMPTY_DIRECTORY_NAME[];

private:

	TCHAR m_szEmptyDirectoryPath[MAX_PATH];
	BOOL m_bDeleteDirectory;
};

const TCHAR CalculateFolderSizeTest::EMPTY_DIRECTORY_NAME[] = L"FolderSizeEmpty";

TEST_F(CalculateFolderSizeTest, Empty)
{
	ULARGE_INTEGER ulTotalFolderSizeExpected;
	ulTotalFolderSizeExpected.QuadPart = 0;
	TestCalculateFolderSize(EMPTY_DIRECTORY_NAME, 0, 0, ulTotalFolderSizeExpected);
}

TEST(CalculateFolderSize, FilesAndFolders)
{
	ULARGE_INTEGER ulTotalFolderSizeExpected;
	ulTotalFolderSizeExpected.QuadPart = 18432;
	TestCalculateFolderSize(L"FolderSize", 2, 6, ulTotalFolderSizeExpected);
}