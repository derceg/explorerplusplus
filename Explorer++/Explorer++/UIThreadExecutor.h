// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/WindowSubclassWrapper.h"
#include <concurrencpp/concurrencpp.h>
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

	static constexpr WCHAR MESSAGE_CLASS_NAME[] = L"MessageClass";

	static HWND CreateMessageOnlyWindow();

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnTaskQueued();
	void OnDestroyWindow();

	const HWND m_hwnd;
	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;
	std::mutex m_mutex;
	std::queue<concurrencpp::task> m_queue;
	std::atomic_bool m_shutdownRequested = false;
};
