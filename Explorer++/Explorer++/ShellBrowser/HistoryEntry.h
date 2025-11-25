// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/Pidl.h"
#include <boost/core/noncopyable.hpp>
#include <vector>

class HistoryEntry : private boost::noncopyable
{
public:
	enum class InitialNavigationType
	{
		// An entry of this type represents a provisional, initial entry. Typically, history entries
		// are only added once a navigation commits. However, since there always needs to be at
		// least one history entry, an entry needs to be created for the very first navigation
		// up-front, before any navigation has been committed.
		//
		// There can be only one initial entry and it will be replaced whenever a commit occurs.
		Initial,

		// Represents a standard, committed entry.
		NonInitial
	};

	HistoryEntry(const PidlAbsolute &pidl,
		InitialNavigationType type = InitialNavigationType::NonInitial);

	int GetId() const;
	const PidlAbsolute &GetPidl() const;
	bool IsInitialEntry() const;
	InitialNavigationType GetInitialNavigationType() const;
	const std::vector<PidlAbsolute> &GetSelectedItems() const;
	void SetSelectedItems(const std::vector<PidlAbsolute> &pidls);

private:
	static inline int idCounter = 0;
	const int m_id;

	const PidlAbsolute m_pidl;
	const InitialNavigationType m_type;
	std::vector<PidlAbsolute> m_selectedItems;
};
