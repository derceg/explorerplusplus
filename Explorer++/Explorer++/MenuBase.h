// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>
#include <vector>

class AcceleratorManager;
class MenuView;

// This class, along with MenuView, is used to implement an MVP menu system. That is, the view is
// considered passive and only contains general display logic. A class that derives from this class
// functions as a presenter and is responsible for retrieving model data and adding it to the view.
class MenuBase
{
public:
	struct IdRange
	{
		UINT startId;
		UINT endId;

		IdRange(UINT startId, UINT endId) : startId(startId), endId(endId)
		{
		}

		// This is only used in tests.
		bool operator==(const IdRange &) const = default;
	};

	MenuBase(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		UINT startId = DEFAULT_START_ID, UINT endId = DEFAULT_END_ID);
	virtual ~MenuBase() = default;

	const IdRange &GetIdRange() const;

protected:
	static constexpr UINT DEFAULT_START_ID = 1;
	static constexpr UINT DEFAULT_END_ID = std::numeric_limits<UINT>::max();

	std::optional<std::wstring> GetAcceleratorTextForId(UINT id) const;

	MenuView *const m_menuView;

private:
	void OnViewDestroyed();

	const AcceleratorManager *const m_acceleratorManager;
	const IdRange m_idRange;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
