// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CachedIcons.h"

CachedIcons::CachedIcons(std::size_t maxItems) : m_maxItems(maxItems)
{
}

void CachedIcons::AddOrUpdateIcon(const std::wstring &itemPath, int iconIndex)
{
	auto [itr, inserted] = m_cachedIconSet.push_front({ itemPath, iconIndex });

	if (inserted)
	{
		if (m_cachedIconSet.size() > m_maxItems)
		{
			m_cachedIconSet.pop_back();
		}
	}
	else
	{
		bool res = m_cachedIconSet.modify(itr,
			[iconIndex](auto &cachedIcon) { cachedIcon.iconIndex = iconIndex; });
		DCHECK(res);

		// Move the icon to the front of the list, which will stop it from being removed if the list
		// grows over the maximum allowed size (the first icons to be removed are those at the back
		// of the list).
		m_cachedIconSet.relocate(m_cachedIconSet.begin(), itr);
	}
}

std::optional<int> CachedIcons::MaybeGetIconIndex(const std::wstring &itemPath)
{
	auto &pathIndex = m_cachedIconSet.get<ByPath>();
	auto itr = pathIndex.find(itemPath);

	if (itr == pathIndex.end())
	{
		return std::nullopt;
	}

	return itr->iconIndex;
}
