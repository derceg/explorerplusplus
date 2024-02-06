// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "OneShotTimerManager.h"
#include "../Helper/WindowSubclassWrapper.h"

OneShotTimerManager::OneShotTimerManager(HWND hwnd) : m_hwnd(hwnd)
{
	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(m_hwnd,
		std::bind_front(&OneShotTimerManager::WndProc, this)));
}

OneShotTimerManager::~OneShotTimerManager() = default;

LRESULT OneShotTimerManager::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_TIMER:
		OnTimer(wParam);
		return 0;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

std::optional<UINT_PTR> OneShotTimerManager::StartTimer(std::chrono::milliseconds delay,
	Callback callback)
{
	UINT_PTR timerId = m_idCounter++;

	auto res = SetTimer(m_hwnd, timerId, static_cast<UINT>(delay.count()), nullptr);

	if (!res)
	{
		LOG_SYSRESULT(GetLastError());
		DCHECK(false);

		return std::nullopt;
	}

	// Timer IDs aren't reused, so there should never be a case where an entry that's inserted
	// overwrites an existing entry.
	auto [itr, didInsert] = m_timerCallbacks.insert({ timerId, callback });
	DCHECK(didInsert);

	return timerId;
}

void OneShotTimerManager::StopTimer(UINT_PTR timerId)
{
	RemoveTimerAndOptionallyRunCallback(timerId, CallbackRunType::DontRun);
}

void OneShotTimerManager::OnTimer(UINT_PTR timerId)
{
	RemoveTimerAndOptionallyRunCallback(timerId, CallbackRunType::Run);
}

void OneShotTimerManager::RemoveTimerAndOptionallyRunCallback(UINT_PTR timerId,
	CallbackRunType runType)
{
	auto itr = m_timerCallbacks.find(timerId);

	if (itr == m_timerCallbacks.end())
	{
		// It's possible for this to legitimately happen. For example, if StopTimer() has been
		// called on a timer that has already run. Or if OnTimer() is called after a timer has
		// already been run.
		return;
	}

	auto callback = itr->second;

	m_timerCallbacks.erase(itr);

	auto res = KillTimer(m_hwnd, timerId);

	if (!res)
	{
		LOG_SYSRESULT(GetLastError());
		DCHECK(false);
	}

	// This is run after the callback has been deleted from the internal map and after the timer has
	// been killed. That's because the callback could potentially enter a nested message loop (e.g.
	// because it shows a dialog) and cause more WM_TIMER messages to be dispatched. In that
	// situation, this function could be called again without first returning. By calling this last,
	// any nested calls to this function will be safe.
	if (runType == CallbackRunType::Run)
	{
		callback();
	}
}
