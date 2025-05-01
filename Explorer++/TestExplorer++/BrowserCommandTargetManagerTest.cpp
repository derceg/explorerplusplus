// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BrowserCommandTargetManager.h"
#include "BrowserCommandTargetFake.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class BrowserCommandTargetManagerTest : public Test
{
protected:
	BrowserCommandTargetManager m_targetManager;
};

TEST_F(BrowserCommandTargetManagerTest, InitialState)
{
	// Should be able to retrieve a default target, even if no target has been set.
	auto *target = m_targetManager.GetCurrentTarget();
	EXPECT_NE(target, nullptr);
}

TEST_F(BrowserCommandTargetManagerTest, SetClear)
{
	BrowserCommandTargetFake target;
	m_targetManager.SetCurrentTarget(&target);
	EXPECT_EQ(m_targetManager.GetCurrentTarget(), &target);

	m_targetManager.ClearCurrentTarget();
	EXPECT_NE(m_targetManager.GetCurrentTarget(), &target);
}

TEST_F(BrowserCommandTargetManagerTest, ChangeSignal)
{
	MockFunction<void()> changeCallback;
	m_targetManager.targetChangedSignal.AddObserver(changeCallback.AsStdFunction());

	MockFunction<void(int)> check;
	{
		InSequence seq;

		EXPECT_CALL(changeCallback, Call());
		EXPECT_CALL(check, Call(1));
		EXPECT_CALL(changeCallback, Call());
	}

	BrowserCommandTargetFake target;
	m_targetManager.SetCurrentTarget(&target);
	check.Call(1);
	m_targetManager.ClearCurrentTarget();
}
