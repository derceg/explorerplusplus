// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "TabNavigationInterface.h"
#include <gmock/gmock.h>

class TabNavigationMock : public TabNavigationInterface
{
public:
	MOCK_METHOD(void, CreateNewTab, (NavigateParams & navigateParams, bool selected), (override));
	MOCK_METHOD(void, SelectTabById, (int tabId), (override));
};
