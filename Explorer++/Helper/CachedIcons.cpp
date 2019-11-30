// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CachedIcons.h"
#include "ShellHelper.h"

CachedIcons::CachedIcons(std::size_t maxItems) :
	m_maxItems(maxItems)
{

}

CachedIcons::iterator CachedIcons::end()
{
	CachedIconSetByPath &pathIndex = m_cachedIconSet.get<1>();
	return pathIndex.end();
}

void CachedIcons::addOrUpdateFileIcon(const std::wstring &filePath, int iconIndex)
{
	auto cachedItr = findByPath(filePath);

	if (cachedItr != end())
	{
		CachedIcon existingCachedIcon = *cachedItr;
		existingCachedIcon.iconIndex = iconIndex;
		replace(cachedItr, existingCachedIcon);
	}
	else
	{
		CachedIcon cachedIcon;
		cachedIcon.filePath = filePath;
		cachedIcon.iconIndex = iconIndex;
		insert(cachedIcon);
	}
}

void CachedIcons::insert(const CachedIcon &cachedIcon)
{
	m_cachedIconSet.push_front(cachedIcon);

	if (m_cachedIconSet.size() > m_maxItems)
	{
		m_cachedIconSet.pop_back();
	}
}

// Replaces an existing cached icon. The icon will also be moved to the
// front of the list, which will stop it from being removed if the list
// grows over the maximum allowed size (the first icons to be removed
// are those at the back of the list).
void CachedIcons::replace(CachedIconSetByPath::iterator itr, const CachedIcon &cachedIcon)
{
	CachedIconSetByPath &pathIndex = m_cachedIconSet.get<1>();
	pathIndex.replace(itr, cachedIcon);

	auto sequenceItr = m_cachedIconSet.iterator_to(*itr);
	m_cachedIconSet.relocate(m_cachedIconSet.begin(), sequenceItr);
}

CachedIcons::iterator CachedIcons::findByPath(const std::wstring &filePath)
{
	CachedIconSetByPath &pathIndex = m_cachedIconSet.get<1>();
	return pathIndex.find(filePath);
}