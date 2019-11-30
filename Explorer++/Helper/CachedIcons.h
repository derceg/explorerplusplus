// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/optional.hpp>

struct CachedIcon
{
	std::wstring filePath;
	int iconIndex;
};

class CachedIcons
{
public:

	typedef boost::multi_index_container<
		CachedIcon,
		boost::multi_index::indexed_by<
			boost::multi_index::sequenced<>,
			boost::multi_index::hashed_unique<boost::multi_index::member<CachedIcon, std::wstring, &CachedIcon::filePath>>
		>
	> CachedIconSet;

	typedef CachedIconSet::nth_index<1>::type CachedIconSetByPath;
	typedef CachedIconSetByPath::iterator iterator;

	CachedIcons(std::size_t maxItems);

	iterator end();

	void addOrUpdateFileIcon(const std::wstring &filePath, int iconIndex);
	void insert(const CachedIcon &cachedIcon);
	void replace(CachedIconSetByPath::iterator itr, const CachedIcon &cachedIcon);
	iterator findByPath(const std::wstring &filePath);

private:

	CachedIconSet m_cachedIconSet;
	std::size_t m_maxItems;
};