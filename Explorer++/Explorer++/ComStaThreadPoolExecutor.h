// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <concurrencpp/concurrencpp.h>
#include <wil/resource.h>
#include <atomic>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

// Represents a pool of threads, where COM has been initialized on each thread with the
// single-threaded apartment model. Each thread will both pump messages and run any queued tasks.
class ComStaThreadPoolExecutor : public concurrencpp::derivable_executor<ComStaThreadPoolExecutor>
{
public:
	ComStaThreadPoolExecutor(int numThreads);

	void enqueue(concurrencpp::task task) override;
	void enqueue(std::span<concurrencpp::task> tasks) override;
	int max_concurrency_level() const noexcept override;
	bool shutdown_requested() const noexcept override;
	void shutdown() noexcept override;

private:
	void ThreadMain();
	void RunLoop();
	bool PerformWork();
	bool PumpMessageLoop();
	bool RunTask();
	void WaitForWork();

	std::vector<std::jthread> m_threads;
	std::mutex m_mutex;
	std::queue<concurrencpp::task> m_queue;
	wil::unique_event_failfast m_taskQueuedEvent;
	wil::unique_event_failfast m_shutDownEvent;
	std::atomic_bool m_shutdownRequested = false;
};
