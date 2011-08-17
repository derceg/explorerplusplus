#include "stdafx.h"
#include <string>
#include "../Helper/Bookmark.h"
#include "../Helper/Macros.h"
#include "gtest\gtest.h"

TEST(BookmarkTest,BookmarkCreation)
{
	std::wstring Name(L"Test name");
	std::wstring Location(L"Test location");
	std::wstring Description(L"Test description");
	CBookmark Bookmark(Name,Location,Description);

	EXPECT_EQ(Name,Bookmark.GetName());
	EXPECT_EQ(Location,Bookmark.GetLocation());
	EXPECT_EQ(Description,Bookmark.GetDescription());
}

TEST(BookmarkTest,BookmarkFolderCreation)
{
	std::wstring Name(L"Test folder name");
	CBookmarkFolder BookmarkFolder = CBookmarkFolder::Create(Name);

	EXPECT_EQ(Name,BookmarkFolder.GetName());
}

TEST(BookmarkTest,BookmarkUpdates)
{
	std::wstring Name(L"Test name");
	std::wstring Location(L"Test location");
	std::wstring Description(L"Test description");
	CBookmark Bookmark(Name,Location,Description);

	std::wstring NewName(L"New test name");
	Bookmark.SetName(NewName);
	EXPECT_EQ(NewName,Bookmark.GetName());

	std::wstring NewLocation(L"New test location");
	Bookmark.SetLocation(NewLocation);
	EXPECT_EQ(NewLocation,Bookmark.GetLocation());

	std::wstring NewDescription(L"New test description");
	Bookmark.SetDescription(NewDescription);
	EXPECT_EQ(NewDescription,Bookmark.GetDescription());
}

TEST(BookmarkTest,BookmarkFolderUpdates)
{
	std::wstring Name(L"Test folder name");
	CBookmarkFolder BookmarkFolder = CBookmarkFolder::Create(Name);

	std::wstring NewName(L"New test folder name");
	BookmarkFolder.SetName(NewName);
	EXPECT_EQ(NewName,BookmarkFolder.GetName());
}

TEST(BookmarkTest,BookmarkChild)
{
	CBookmarkFolder BookmarkFolderParent = CBookmarkFolder::Create(L"Test");
	CBookmark Bookmark(L"Test name",L"Test location",L"Test description");

	BookmarkFolderParent.InsertBookmark(Bookmark);

	EXPECT_EQ(false,BookmarkFolderParent.HasChildFolder());

	auto itr = BookmarkFolderParent.begin();

	CBookmark ChildBookmark = boost::get<CBookmark>(*itr);

	EXPECT_EQ(Bookmark.GetName(),ChildBookmark.GetName());
	EXPECT_EQ(Bookmark.GetLocation(),ChildBookmark.GetLocation());
	EXPECT_EQ(Bookmark.GetDescription(),ChildBookmark.GetDescription());
}

TEST(BookmarkTest,BookmarkFolderChild)
{
	CBookmarkFolder BookmarkFolderParent = CBookmarkFolder::Create(L"Test");
	CBookmarkFolder BookmarkFolder = CBookmarkFolder::Create(L"Test name");

	BookmarkFolderParent.InsertBookmarkFolder(BookmarkFolder);

	ASSERT_EQ(true,BookmarkFolderParent.HasChildFolder());

	auto itr = BookmarkFolderParent.begin();

	CBookmarkFolder ChildBookmarkFolder = boost::get<CBookmarkFolder>(*itr);

	EXPECT_EQ(BookmarkFolder.GetName(),ChildBookmarkFolder.GetName());
}

class CTestBookmarkItemNotifier : public NBookmark::IBookmarkItemNotification
{
public:

	void	OnBookmarkItemModified(const GUID &guid);
	void	OnBookmarkAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmark &Bookmark);
	void	OnBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmarkFolder &BookmarkFolder);
	void	OnBookmarkRemoved(const GUID &guid);
	void	OnBookmarkFolderRemoved(const GUID &guid);
};

void CTestBookmarkItemNotifier::OnBookmarkItemModified(const GUID &guid)
{

}

void CTestBookmarkItemNotifier::OnBookmarkAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmark &Bookmark)
{
	EXPECT_EQ(std::wstring(L"Test name"),Bookmark.GetName());
	EXPECT_EQ(std::wstring(L"Test location"),Bookmark.GetLocation());
	EXPECT_EQ(std::wstring(L"Test description"),Bookmark.GetDescription());
}

void CTestBookmarkItemNotifier::OnBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmarkFolder &BookmarkFolder)
{

}

void CTestBookmarkItemNotifier::OnBookmarkRemoved(const GUID &guid)
{

}

void CTestBookmarkItemNotifier::OnBookmarkFolderRemoved(const GUID &guid)
{

}

TEST(BookmarkTest,UpdateNotifications)
{
	CBookmarkFolder BookmarkFolderParent = CBookmarkFolder::Create(L"Test");
	CBookmark Bookmark(L"Test name",L"Test location",L"Test description");

	CTestBookmarkItemNotifier *ptbn = new CTestBookmarkItemNotifier();
	CBookmarkItemNotifier::GetInstance().AddObserver(ptbn);

	BookmarkFolderParent.InsertBookmark(Bookmark);
}