// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "WindowStorageTestHelper.h"
#include "WindowStorage.h"
#include <boost/pfr.hpp>

bool operator==(const WindowStorageData &first, const WindowStorageData &second)
{
	using WindowStorageDataTuple =
		decltype(boost::pfr::structure_to_tuple(std::declval<WindowStorageData>()));
	static_assert(std::tuple_size_v<WindowStorageDataTuple> == 2);

	return boost::pfr::structure_to_tuple(first.bounds)
		== boost::pfr::structure_to_tuple(second.bounds)
		&& first.showState == second.showState;
}

namespace WindowStorageTestHelper
{

std::vector<WindowStorageData> BuildV2ReferenceWindows()
{
	return { { { 618, 598, 1825, 1249 }, WindowShowState::Normal },
		{ { 212, 40, 400, 1073 }, WindowShowState::Minimized },
		{ { 1165, 2, 2071, 643 }, WindowShowState::Maximized } };
}

WindowStorageData BuildV1ReferenceWindow()
{
	return { { 98, 87, 1606, 798 }, WindowShowState::Normal };
}

}
