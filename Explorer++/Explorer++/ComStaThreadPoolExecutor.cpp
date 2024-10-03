// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ComStaThreadPoolExecutor.h"

ComStaThreadPoolExecutor::ComStaThreadPoolExecutor(int numThreads) :
	concurrencpp::derivable_executor<ComStaThreadPoolExecutor>("ComStaThreadPoolExecutor")
{
	for (int i = 0; i < numThreads; i++)
	{
		auto thread = std::jthread(&ComStaThreadPoolExecutor::ThreadMain, this);
		m_threads.push_back(std::move(thread));
	}

	m_taskQueuedEvent.create(wil::EventOptions::None);

	// This is set up as a manual reset event, since once shutdown is requested, this event should
	// be left permanently set.
	m_shutDownEvent.create(wil::EventOptions::ManualReset);
}

void ComStaThreadPoolExecutor::enqueue(concurrencpp::task task)
{
	std::span<concurrencpp::task> taskSpan(&task, 1);
	enqueue(taskSpan);
}

void ComStaThreadPoolExecutor::enqueue(std::span<concurrencpp::task> tasks)
{
	if (m_shutdownRequested)
	{
		throw concurrencpp::errors::runtime_shutdown("COM STA executor already shut down");
	}

	std::unique_lock<std::mutex> lock(m_mutex);

	for (auto &task : tasks)
	{
		m_queue.emplace(std::move(task));
	}

	lock.unlock();

	// Note that this will only wake a single thread, which is problematic if multiple tasks are
	// being queued at once. In practice, that shouldn't be an issue, since the application really
	// only needs to queue one task at a time. Although this overloaded version of enqueue() is
	// present, it only exists to satisfy the interface declared by concurrencpp::executor.
	m_taskQueuedEvent.SetEvent();
}

int ComStaThreadPoolExecutor::max_concurrency_level() const noexcept
{
	return static_cast<int>(m_threads.size());
}

bool ComStaThreadPoolExecutor::shutdown_requested() const noexcept
{
	return m_shutdownRequested;
}

void ComStaThreadPoolExecutor::shutdown() noexcept
{
	const auto shutdownRequested = m_shutdownRequested.exchange(true);

	if (shutdownRequested)
	{
		return;
	}

	std::unique_lock<std::mutex> lock(m_mutex);
	m_queue = {};
	lock.unlock();

	m_shutDownEvent.SetEvent();

	for (auto &thread : m_threads)
	{
		thread.join();
	}
}

void ComStaThreadPoolExecutor::ThreadMain()
{
	auto comInitialization = wil::CoInitializeEx_failfast(COINIT_APARTMENTTHREADED);

	// This will force the system to create the message queue.
	MSG msg;
	PeekMessage(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);

	RunLoop();
}

void ComStaThreadPoolExecutor::RunLoop()
{
	while (!m_shutdownRequested)
	{
		while (PerformWork())
			;

		WaitForWork();
	}
}

bool ComStaThreadPoolExecutor::PerformWork()
{
	if (PumpMessageLoop())
	{
		return true;
	}

	if (RunTask())
	{
		return true;
	}

	return false;
}

bool ComStaThreadPoolExecutor::PumpMessageLoop()
{
	MSG msg;
	BOOL res = PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);

	if (!res)
	{
		return false;
	}

	TranslateMessage(&msg);
	DispatchMessage(&msg);

	return true;
}

bool ComStaThreadPoolExecutor::RunTask()
{
	std::unique_lock<std::mutex> lock(m_mutex);

	if (m_queue.empty())
	{
		return false;
	}

	auto task = std::move(m_queue.front());
	m_queue.pop();

	lock.unlock();

	task();

	return true;
}

void ComStaThreadPoolExecutor::WaitForWork()
{
	HANDLE handles[] = { m_taskQueuedEvent.get(), m_shutDownEvent.get() };
	MsgWaitForMultipleObjectsEx(std::size(handles), handles, INFINITE, QS_ALLINPUT, 0);
}
