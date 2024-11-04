// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RegistryAppStorage.h"
#include "Bookmarks/BookmarkRegistryStorage.h"

RegistryAppStorage::RegistryAppStorage(wil::unique_hkey applicationKey) :
	m_applicationKey(std::move(applicationKey))
{
}

void RegistryAppStorage::LoadBookmarks(BookmarkTree *bookmarkTree)
{
	BookmarkRegistryStorage::Load(m_applicationKey.get(), bookmarkTree);
}
