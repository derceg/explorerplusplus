// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "UIThreadExecutor.h"
#include "../Helper/MessageWindowHelper.h"
#include "../Helper/WindowHelper.h"

UIThreadExecutor::UIThreadExecutor() :
	concurrencpp::derivable_executor<UIThreadExecutor>("UIThreadExecutor"),
	m_hwnd(MessageWindowHelper::CreateMessageOnlyWindow())
{
	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(m_hwnd.get(),
		std::bind_front(&UIThreadExecutor::WndProc, this)));
}

void UIThreadExecutor::enqueue(concurrencpp::task task)
{
	std::span<concurrencpp::task> taskSpan(&task, 1);
	enqueue(taskSpan);
}

void UIThreadExecutor::enqueue(std::span<concurrencpp::task> tasks)
{
	if (m_shutdownRequested)
	{
		throw concurrencpp::errors::runtime_shutdown("UI thread executor already shut down");
	}

	std::unique_lock<std::mutex> lock(m_mutex);

	for (auto &task : tasks)
	{
		m_queue.emplace(std::move(task));
	}

	lock.unlock();

	PostMessage(m_hwnd.get(), WM_USER_TASK_QUEUED, 0, 0);
}

int UIThreadExecutor::max_concurrency_level() const noexcept
{
	return 1;
}

bool UIThreadExecutor::shutdown_requested() const noexcept
{
	return m_shutdownRequested;
}

void UIThreadExecutor::shutdown() noexcept
{
	const auto shutdownRequested = m_shutdownRequested.exchange(true);

	if (shutdownRequested)
	{
		return;
	}

	std::unique_lock<std::mutex> lock(m_mutex);
	m_queue = {};
	lock.unlock();

	auto res = SendMessage(m_hwnd.get(), WM_USER_DESTROY_WINDOW, 0, 0);
	DCHECK_EQ(res, 1);
}

LRESULT UIThreadExecutor::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_USER_TASK_QUEUED:
		OnTaskQueued();
		return 1;

	case WM_USER_DESTROY_WINDOW:
		m_hwnd.reset();
		return 1;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void UIThreadExecutor::OnTaskQueued()
{
	if (m_runningTask)
	{
		// Task nesting isn't allowed, since it can cause subtle reentrancy issues. For example, if
		// nesting was allowed, the following situation would be possible:
		//
		// 1. Two tasks are submitted.
		// 2. Task 1 is run.
		// 3. Task 1 starts a nested message loop (e.g. by showing a dialog).
		// 4. The nested message loop results in this method being invoked, leading to task 2
		//    running in the middle of task 1.
		//
		// To avoid issues like that, only one task can be run at a time.
		//
		// Note that the loop below will process all tasks in the queue, so the early return here
		// doesn't matter - all remaining tasks will be run when control returns to the outer loop.
		return;
	}

	while (auto task = GetNextTask())
	{
		if (m_shutdownRequested)
		{
			return;
		}

		m_runningTask = true;
		task();
		m_runningTask = false;
	}
}

concurrencpp::task UIThreadExecutor::GetNextTask()
{
	std::unique_lock<std::mutex> lock(m_mutex);

	if (m_queue.empty())
	{
		return {};
	}

	auto task = std::move(m_queue.front());
	m_queue.pop();

	return task;
}
