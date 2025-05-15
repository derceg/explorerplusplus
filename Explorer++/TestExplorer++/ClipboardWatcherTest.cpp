// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/ClipboardWatcher.h"
#include "ExecutorWrapper.h"
#include "MessageLoop.h"
#include "UIThreadExecutor.h"
#include "../Helper/SystemClipboard.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace std::chrono_literals;
using namespace testing;

class ClipboardWatcherTest : public Test
{
protected:
	ClipboardWatcherTest() : m_uiThreadExecutor(std::make_shared<UIThreadExecutor>())
	{
		m_clipboardWatcher.updateSignal.AddObserver(
			[this]
			{
				m_callback.Call();
				m_messageLoop.Stop();
			});
	}

	void WaitForUpdate()
	{
		// ClipboardWatcher observes clipboard updates by handling the WM_CLIPBOARDUPDATE message.
		// Therefore, the message loop will need to be pumped for that message to be received.
		m_messageLoop.RunWithTimeout(5s, m_uiThreadExecutor.Get());
	}

	MessageLoop m_messageLoop;
	ExecutorWrapper<UIThreadExecutor> m_uiThreadExecutor;
	MockFunction<void()> m_callback;
	ClipboardWatcher m_clipboardWatcher;
};

TEST_F(ClipboardWatcherTest, UpdateOnWrite)
{
	EXPECT_CALL(m_callback, Call());

	{
		SystemClipboard clipboard;
		clipboard.WriteText(L"Text");
	}

	WaitForUpdate();
}

TEST_F(ClipboardWatcherTest, UpdateOnClear)
{
	EXPECT_CALL(m_callback, Call());

	{
		SystemClipboard clipboard;
		clipboard.Clear();
	}

	WaitForUpdate();
}
