// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "UIThreadExecutor.h"
#include "MessageWindowHelper.h"
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
	std::queue<concurrencpp::task> localQueue;

	std::unique_lock<std::mutex> lock(m_mutex);
	std::swap(localQueue, m_queue);
	lock.unlock();

	while (!localQueue.empty())
	{
		if (m_shutdownRequested)
		{
			return;
		}

		auto task = std::move(localQueue.front());
		localQueue.pop();

		task();
	}
}
