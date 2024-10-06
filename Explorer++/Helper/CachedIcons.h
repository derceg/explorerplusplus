// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>

class CachedIcons
{
public:
	CachedIcons(std::size_t maxItems);

	void AddOrUpdateIcon(const std::wstring &itemPath, int iconIndex);
	std::optional<int> MaybeGetIconIndex(const std::wstring &itemPath);

private:
	struct CachedIcon
	{
		std::wstring itemPath;
		int iconIndex;
	};

	struct ByInsertionOrder
	{
	};

	struct ByPath
	{
	};

	// clang-format off
	using CachedIconSet = boost::multi_index_container<CachedIcon,
		boost::multi_index::indexed_by<
			// An index of items, sorted by their insertion order.
			boost::multi_index::sequenced<
				boost::multi_index::tag<ByInsertionOrder>
			>,

			// A non-sorted index of items, based on the item path.
			boost::multi_index::hashed_unique<
				boost::multi_index::tag<ByPath>,
				boost::multi_index::member<CachedIcon, std::wstring, &CachedIcon::itemPath>
			>
		>
	>;
	// clang-format on

	CachedIconSet m_cachedIconSet;
	const std::size_t m_maxItems;
};
