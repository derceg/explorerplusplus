/******************************************************************
 *
 * Project: Helper
 * File: Bookmark.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Implements a general bookmarking system.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "Bookmark.h"
#include "RegistrySettings.h"


using namespace std;

#define ROOT_NAME	_T("Bookmarks")

CBookmark::CBookmark(void)
{
	/* Fill out information for the root. */
	StringCchCopy(m_Root.szItemName,SIZEOF_ARRAY(m_Root.szItemName),ROOT_NAME);
	m_Root.Type			= BOOKMARK_TYPE_FOLDER;
	m_Root.NextSibling	= NULL;
	m_Root.FirstChild	= NULL;
}

CBookmark::~CBookmark(void)
{
}

void CBookmark::GetRoot(Bookmark_t *pRoot)
{
	StringCchCopy(pRoot->szItemName,SIZEOF_ARRAY(pRoot->szItemName),
		m_Root.szItemName);
	StringCchCopy(pRoot->szItemDescription,
		SIZEOF_ARRAY(pRoot->szItemDescription),
		m_Root.szItemDescription);

	pRoot->Type				= m_Root.Type;
	pRoot->bShowOnToolbar	= m_Root.bShowOnToolbar;
	pRoot->pHandle			= (void *)&m_Root;
}

HRESULT CBookmark::GetChild(Bookmark_t *pParent,Bookmark_t *pChild)
{
	BookmarkInternal_t	*pbi = NULL;
	HRESULT				hr = E_FAIL;

	pbi = (BookmarkInternal_t *)pParent->pHandle;

	/* First check if this item is actually
	a folder. */
	if(pbi->Type == BOOKMARK_TYPE_FOLDER)
	{
		/* Does this folder have any children?
		If it does, copy the bookmark information
		across. */
		if(pbi->FirstChild != NULL)
		{
			ExportBookmarkInternal((BookmarkInternal_t *)pbi->FirstChild,pChild);

			hr = S_OK;
		}
	}

	return hr;
}

HRESULT CBookmark::GetChildFolder(Bookmark_t *pParent,Bookmark_t *pChildFolder)
{
	BookmarkInternal_t	*pbi = NULL;
	BookmarkInternal_t	*pChild = NULL;
	HRESULT				hr = E_FAIL;

	pbi = (BookmarkInternal_t *)pParent->pHandle;

	/* First check if this item is actually
	a folder. */
	if(pbi->Type == BOOKMARK_TYPE_FOLDER)
	{
		/* Does this folder have any folder children?
		If it does, copy the bookmark information
		across. */
		pChild = (BookmarkInternal_t *)pbi->FirstChild;

		while(pChild != NULL && pChild->Type != BOOKMARK_TYPE_FOLDER)
			pChild = (BookmarkInternal_t *)pChild->NextSibling;

		if(pChild != NULL)
		{
			ExportBookmarkInternal(pChild,pChildFolder);

			hr = S_OK;
		}
	}

	return hr;
}

HRESULT CBookmark::GetNextBookmarkSibling(Bookmark_t *pParent,Bookmark_t *pSibling)
{
	BookmarkInternal_t	*pbi = NULL;
	HRESULT				hr = E_FAIL;

	pbi = (BookmarkInternal_t *)pParent->pHandle;

	/* Does this item have any siblings?
	If it does, copy the bookmark information
	across. */
	if(pbi->NextSibling != NULL)
	{
		ExportBookmarkInternal((BookmarkInternal_t *)pbi->NextSibling,pSibling);

		hr = S_OK;
	}

	return hr;
}

HRESULT CBookmark::GetNextFolderSibling(Bookmark_t *pParent,Bookmark_t *pFolderSibling)
{
	BookmarkInternal_t	*pbi = NULL;
	BookmarkInternal_t	*pSibling = NULL;
	HRESULT				hr = E_FAIL;

	pbi = (BookmarkInternal_t *)pParent->pHandle;

	/* Does this item have any folder siblings?
	If it does, copy the bookmark information
	across. */
	pSibling = (BookmarkInternal_t *)pbi->NextSibling;

	while(pSibling != NULL && pSibling->Type != BOOKMARK_TYPE_FOLDER)
		pSibling = (BookmarkInternal_t *)pSibling->NextSibling;

	if(pSibling != NULL)
	{
		ExportBookmarkInternal(pSibling,pFolderSibling);

		hr = S_OK;
	}

	return hr;
}

void CBookmark::ImportBookmarkInternal(BookmarkInternal_t *pbi,Bookmark_t *pBookmark)
{
	StringCchCopy(pbi->szItemName,SIZEOF_ARRAY(pbi->szItemName),
		pBookmark->szItemName);
	StringCchCopy(pbi->szItemDescription,
		SIZEOF_ARRAY(pbi->szItemDescription),
		pBookmark->szItemDescription);

	pbi->Type			= pBookmark->Type;
	pbi->bShowOnToolbar	= pBookmark->bShowOnToolbar;
	pbi->FirstChild		= NULL;
	pbi->NextSibling	= NULL;

	if(pBookmark->Type == BOOKMARK_TYPE_BOOKMARK)
	{
		/* If this is a bookmark, also copy
		across its location. The calling code
		will be responsible for freeing this. */
		StringCchCopy(pbi->szLocation,SIZEOF_ARRAY(pbi->szLocation),
			pBookmark->szLocation);
	}

	pBookmark->pHandle	= (void *)pbi;
}

void CBookmark::ExportBookmarkInternal(BookmarkInternal_t *pbi,Bookmark_t *pBookmark)
{
	StringCchCopy(pBookmark->szItemName,SIZEOF_ARRAY(pBookmark->szItemName),
		pbi->szItemName);
	StringCchCopy(pBookmark->szItemDescription,
		SIZEOF_ARRAY(pBookmark->szItemDescription),
		pbi->szItemDescription);

	pBookmark->Type				= pbi->Type;
	pBookmark->bShowOnToolbar	= pbi->bShowOnToolbar;

	if(pbi->Type == BOOKMARK_TYPE_BOOKMARK)
	{
		/* If this is a bookmark, also copy
		across its location. The calling code
		will be responsible for freeing this. */
		StringCchCopy(pBookmark->szLocation,
			SIZEOF_ARRAY(pBookmark->szLocation),pbi->szLocation);
	}

	pBookmark->pHandle	= (void *)pbi;
}

void CBookmark::RetrieveBookmark(void *pBookmarkHandle,Bookmark_t *pBookmark)
{
	ExportBookmarkInternal((BookmarkInternal_t *)pBookmarkHandle,pBookmark);
}

void CBookmark::CreateNewBookmark(void *pParentHandle,Bookmark_t *pFolder)
{
	BookmarkInternal_t	*pbiParent = NULL;
	BookmarkInternal_t	*pNewBookmark = NULL;
	BookmarkInternal_t	*pChild = NULL;

	pbiParent = (BookmarkInternal_t *)pParentHandle;

	pNewBookmark = (BookmarkInternal_t *)malloc(sizeof(BookmarkInternal_t));

	ImportBookmarkInternal(pNewBookmark,pFolder);

	pNewBookmark->Parent = (void *)pbiParent;

	if(pbiParent->FirstChild == NULL)
	{
		pbiParent->FirstChild			= pNewBookmark;
		pNewBookmark->PreviousSibling	= NULL;
	}
	else
	{
		pChild = (BookmarkInternal_t *)pbiParent->FirstChild;

		while(pChild->NextSibling != NULL)
		{
			pChild = (BookmarkInternal_t *)pChild->NextSibling;
		}

		pChild->NextSibling				= pNewBookmark;
		pNewBookmark->PreviousSibling	= pChild;
	}
}

void CBookmark::DeleteBookmark(void *pBookmarkHandle)
{
	BookmarkInternal_t	*pbiBookmark = NULL;
	BookmarkInternal_t	*pChild = NULL;

	pbiBookmark = (BookmarkInternal_t *)pBookmarkHandle;

	/* If this is a bookmark, it can just be deleted.
	If it is a folder, all its children will need
	to be deleted as well. */
	if(pbiBookmark->Type == BOOKMARK_TYPE_FOLDER)
	{
		pChild = (BookmarkInternal_t *)pbiBookmark->FirstChild;

		while(pChild->NextSibling != NULL)
		{
			DeleteBookmark((void *)pChild);

			pChild = (BookmarkInternal_t *)pChild->NextSibling;
		}
	}

	/* Now, remap the pointers for the parent
	item. The previous sibling (if any) will
	will have it's next pointer remapped
	to the next sibling from the current
	item (if any). */
	if(pbiBookmark->PreviousSibling == NULL)
	{
		((BookmarkInternal_t *)pbiBookmark->Parent)->FirstChild = pbiBookmark->NextSibling;

		/* If there is a next sibling, set its previous pointer. */
		if(pbiBookmark->NextSibling != NULL)
			((BookmarkInternal_t *)pbiBookmark->NextSibling)->PreviousSibling = NULL;
	}
	else
	{
		((BookmarkInternal_t *)pbiBookmark->PreviousSibling)->NextSibling = pbiBookmark->NextSibling;

		/* If there is a next sibling, set its previous pointer. */
		if(pbiBookmark->NextSibling != NULL)
			((BookmarkInternal_t *)pbiBookmark->NextSibling)->PreviousSibling = pbiBookmark->PreviousSibling;
	}

	free(pbiBookmark);
}

/* ONLY properties that can be updated are:
 - Name
 - Location (if this item is a bookmark)
 - Description
 - Show on toolbar status
No other fields may be changed externally. */
void CBookmark::UpdateBookmark(void *pBookmarkHandle,Bookmark_t *pUpdatedBookmark)
{
	BookmarkInternal_t *pbi;

	pbi = (BookmarkInternal_t *)pBookmarkHandle;

	StringCchCopy(pbi->szItemName,SIZEOF_ARRAY(pbi->szItemName),
		pUpdatedBookmark->szItemName);
	StringCchCopy(pbi->szItemDescription,
		SIZEOF_ARRAY(pbi->szItemDescription),
		pUpdatedBookmark->szItemDescription);

	pbi->bShowOnToolbar	= pUpdatedBookmark->bShowOnToolbar;

	if(pbi->Type == BOOKMARK_TYPE_BOOKMARK)
	{
		/* If this is a bookmark, also copy
		across its location. */
		StringCchCopy(pbi->szLocation,SIZEOF_ARRAY(pbi->szLocation),
			pUpdatedBookmark->szLocation);
	}

	pUpdatedBookmark->Type		= pbi->Type;
	pUpdatedBookmark->pHandle	= pBookmarkHandle;
}

void CBookmark::SwapBookmarks(Bookmark_t *pBookmark1,Bookmark_t *pBookmark2)
{
	BookmarkInternal_t	*pbi1 = NULL;
	BookmarkInternal_t	*pbi2 = NULL;
	BookmarkInternal_t	biTemp;

	pbi1 = (BookmarkInternal_t *)pBookmark1->pHandle;
	pbi2 = (BookmarkInternal_t *)pBookmark2->pHandle;

	/* Note that the previous and next sibling members
	DO NOT need to be altered, since the members in each
	element are simply been swapped. */

	StringCchCopy(biTemp.szItemName,SIZEOF_ARRAY(biTemp.szItemName),
		pbi1->szItemName);
	StringCchCopy(biTemp.szItemDescription,SIZEOF_ARRAY(biTemp.szItemDescription),
		pbi1->szItemDescription);
	StringCchCopy(biTemp.szLocation,SIZEOF_ARRAY(biTemp.szLocation),
		pbi1->szLocation);
	biTemp.bShowOnToolbar	= pbi1->bShowOnToolbar;
	biTemp.Type				= pbi1->Type;
	biTemp.Parent			= pbi1->Parent;
	biTemp.FirstChild		= pbi1->FirstChild;

	StringCchCopy(pbi1->szItemName,SIZEOF_ARRAY(pbi1->szItemName),
		pbi2->szItemName);
	StringCchCopy(pbi1->szItemDescription,SIZEOF_ARRAY(pbi1->szItemDescription),
		pbi2->szItemDescription);
	StringCchCopy(pbi1->szLocation,SIZEOF_ARRAY(pbi1->szLocation),
		pbi2->szLocation);
	pbi1->bShowOnToolbar	= pbi2->bShowOnToolbar;
	pbi1->Type				= pbi2->Type;
	pbi1->Parent			= pbi2->Parent;
	pbi1->FirstChild		= pbi2->FirstChild;

	StringCchCopy(pbi2->szItemName,SIZEOF_ARRAY(pbi2->szItemName),
		biTemp.szItemName);
	StringCchCopy(pbi2->szItemDescription,SIZEOF_ARRAY(pbi2->szItemDescription),
		biTemp.szItemDescription);
	StringCchCopy(pbi2->szLocation,SIZEOF_ARRAY(pbi2->szLocation),
		biTemp.szLocation);
	pbi2->bShowOnToolbar	= biTemp.bShowOnToolbar;
	pbi2->Type				= biTemp.Type;
	pbi2->Parent			= biTemp.Parent;
	pbi2->FirstChild		= biTemp.FirstChild;
}