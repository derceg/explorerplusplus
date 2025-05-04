// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ScopedBrowserCommandTarget.h"
#include "BrowserCommandTargetFake.h"
#include "BrowserCommandTargetManager.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include <gtest/gtest.h>

class ScopedBrowserCommandTargetTest : public BrowserTestBase
{
protected:
	ScopedBrowserCommandTargetTest() :
		m_browser(AddBrowser()),
		m_targetManager(m_browser->GetCommandTargetManager())
	{
	}

	BrowserWindowFake *const m_browser;
	BrowserCommandTargetManager *const m_targetManager;
};

TEST_F(ScopedBrowserCommandTargetTest, SetClear)
{
	BrowserCommandTargetFake target;
	auto scopedTarget = std::make_unique<ScopedBrowserCommandTarget>(m_targetManager, &target);
	EXPECT_NE(m_targetManager->GetCurrentTarget(), &target);

	scopedTarget->TargetFocused();
	EXPECT_EQ(m_targetManager->GetCurrentTarget(), &target);

	scopedTarget.reset();
	EXPECT_NE(m_targetManager->GetCurrentTarget(), &target);
}
