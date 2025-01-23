// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/WindowSubclass.h"
#include <concurrencpp/concurrencpp.h>
#include <wil/resource.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

class UIThreadExecutor : public concurrencpp::derivable_executor<UIThreadExecutor>
{
public:
	UIThreadExecutor();

	void enqueue(concurrencpp::task task) override;
	void enqueue(std::span<concurrencpp::task> tasks) override;
	int max_concurrency_level() const noexcept override;
	bool shutdown_requested() const noexcept override;
	void shutdown() noexcept override;

private:
	static constexpr UINT WM_USER_TASK_QUEUED = WM_USER;
	static constexpr UINT WM_USER_DESTROY_WINDOW = WM_USER + 1;

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnTaskQueued();
	concurrencpp::task GetNextTask();

	wil::unique_hwnd m_hwnd;
	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
	std::mutex m_mutex;
	std::queue<concurrencpp::task> m_queue;
	bool m_runningTask = false;
	std::atomic_bool m_shutdownRequested = false;
};
