// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/NavigationEvents.h"
#include "TabEvents.h"
#include <gtest/gtest.h>
#include <memory>
#include <vector>

class BrowserWindowFake;

class BrowserTestBase : public testing::Test
{
protected:
	BrowserWindowFake *AddBrowser();
	void RemoveBrowser(const BrowserWindowFake *browser);

	TabEvents m_tabEvents;
	NavigationEvents m_navigationEvents;

private:
	std::vector<std::unique_ptr<BrowserWindowFake>> m_browsers;
};
