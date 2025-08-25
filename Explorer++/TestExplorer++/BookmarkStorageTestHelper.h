// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class BookmarkTree;

void BuildV2LoadSaveReferenceTree(BookmarkTree *bookmarkTree);
void BuildV1BasicLoadReferenceTree(BookmarkTree *bookmarkTree);
void BuildV1NestedShowOnToolbarLoadReferenceTree(BookmarkTree *bookmarkTree);
void CompareBookmarkTrees(const BookmarkTree *firstTree, const BookmarkTree *secondTree,
	bool compareGuids);
void PerformV2UpdateObserverInvokedOnceTest(BookmarkTree *loadedBookmarkTree);
void PerformV1UpdateObserverInvokedOnceTest(BookmarkTree *loadedBookmarkTree);
