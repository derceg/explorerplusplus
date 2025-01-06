// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/PidlHelper.h"
#include <boost/core/noncopyable.hpp>
#include <vector>

class HistoryEntry : private boost::noncopyable
{
public:
	HistoryEntry(const PidlAbsolute &pidl);

	int GetId() const;
	const PidlAbsolute &GetPidl() const;
	const std::vector<PidlAbsolute> &GetSelectedItems() const;
	void SetSelectedItems(const std::vector<PidlAbsolute> &pidls);

private:
	static inline int idCounter = 0;
	const int m_id;

	const PidlAbsolute m_pidl;
	std::vector<PidlAbsolute> m_selectedItems;
};
