// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "WindowStorageTestHelper.h"
#include "ShellTestHelper.h"
#include "TabStorage.h"
#include "TabStorageTestHelper.h"
#include "WindowStorage.h"
#include <boost/pfr.hpp>

bool operator==(const WindowStorageData &first, const WindowStorageData &second)
{
	using WindowStorageDataTuple =
		decltype(boost::pfr::structure_to_tuple(std::declval<WindowStorageData>()));
	static_assert(std::tuple_size_v<WindowStorageDataTuple> == 4);

	return boost::pfr::structure_to_tuple(first.bounds)
		== boost::pfr::structure_to_tuple(second.bounds)
		&& first.showState == second.showState && first.tabs == second.tabs
		&& first.selectedTab == second.selectedTab;
}

namespace
{

TabStorageData CreateTabStorageFromDirectory(const std::wstring &directory)
{
	return { CreateSimplePidlForTest(directory), directory };
}

}

namespace WindowStorageTestHelper
{

std::vector<WindowStorageData> BuildV2ReferenceWindows()
{
	// clang-format off
	return {
		{
			{ 618, 598, 1825, 1249 },
			WindowShowState::Normal,
			{
				CreateTabStorageFromDirectory(L"c:\\fake1")
			},
			0
		},
		{
			{ 212, 40, 400, 1073 },
			WindowShowState::Minimized,
			{
				CreateTabStorageFromDirectory(L"c:\\fake1"),
				CreateTabStorageFromDirectory(L"c:\\fake2"),
				CreateTabStorageFromDirectory(L"c:\\fake3")
			},
			2
		},
		{
			{ 1165, 2, 2071, 643 },
			WindowShowState::Maximized,
			{
				CreateTabStorageFromDirectory(L"c:\\fake1"),
				CreateTabStorageFromDirectory(L"c:\\fake2")
			},
			1
		}
	};
	// clang-format on
}

WindowStorageData BuildV2FallbackReferenceWindow()
{
	// clang-format off
	return {
		{ 683, 790, 1073, 2280 },
		WindowShowState::Normal,
		{
			CreateTabStorageFromDirectory(L"c:\\fake1"),
			CreateTabStorageFromDirectory(L"c:\\fake2"),
			CreateTabStorageFromDirectory(L"c:\\fake3")
		},
		2
	};
	// clang-format on
}

WindowStorageData BuildV1ReferenceWindow()
{
	// clang-format off
	return {
		{ 98, 87, 1606, 798 },
		WindowShowState::Normal,
		{
			CreateTabStorageFromDirectory(L"c:\\fake1"),
			CreateTabStorageFromDirectory(L"c:\\fake2")
		},
		1
	};
	// clang-format on
}

}
