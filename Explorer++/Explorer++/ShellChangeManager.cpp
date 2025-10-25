// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellChangeManager.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/StringHelper.h"
#include "../Helper/WindowSubclass.h"

ShellChangeManager::~ShellChangeManager()
{
	// If a watch hasn't been stopped by this point, it means a client has a dangling pointer
	// somewhere. In other words, each client requires a pointer to this class to be able to call
	// StopWatching(). If a client hasn't called that method by the time this class is destroyed, it
	// means that the client has a dangling pointer to this class.
	CHECK(m_messageIdToWatchDetailsMap.empty());
}

std::optional<UINT> ShellChangeManager::StartWatching(const PidlAbsolute &pidl, LONG events,
	Callback callback, bool recursive)
{
	EnsureWindow();

	UINT messageId = GetNextMessageId();

	SHChangeNotifyEntry entry;
	entry.pidl = pidl.Raw();
	entry.fRecursive = recursive;
	ULONG changeNotifyId = SHChangeNotifyRegister(m_hwnd.get(),
		SHCNRF_ShellLevel | SHCNRF_InterruptLevel | SHCNRF_NewDelivery, events, messageId, 1,
		&entry);

	if (changeNotifyId == 0)
	{
		std::wstring path;
		HRESULT hr = GetDisplayName(pidl.Raw(), SHGDN_FORPARSING, path);

		if (SUCCEEDED(hr))
		{
			LOG(WARNING) << "Couldn't monitor directory \"" << wstrToUtf8Str(path)
						 << "\" for changes.";
		}

		return std::nullopt;
	}

	auto [itr, didInsert] =
		m_messageIdToWatchDetailsMap.insert({ messageId, { changeNotifyId, callback } });
	CHECK(didInsert);

	return messageId;
}

void ShellChangeManager::StopWatching(UINT id)
{
	auto itr = m_messageIdToWatchDetailsMap.find(id);
	CHECK(itr != m_messageIdToWatchDetailsMap.end());

	auto res = SHChangeNotifyDeregister(itr->second.changeNotifyId);
	CHECK(res);

	m_messageIdToWatchDetailsMap.erase(itr);
}

UINT ShellChangeManager::GetNextMessageId()
{
	UINT id = m_messageIdCounter;

	m_messageIdCounter++;

	if (m_messageIdCounter > MESSAGE_ID_MAX)
	{
		m_messageIdCounter = MESSAGE_ID_MIN;
	}

	return id;
}

void ShellChangeManager::EnsureWindow()
{
	if (m_hwnd)
	{
		return;
	}

	static bool classRegistered = false;

	if (!classRegistered)
	{
		auto res = RegisterWindowClass();
		CHECK_NE(res, 0);

		classRegistered = true;
	}

	m_hwnd.reset(CreateWindow(CLASS_NAME, L"", WS_DISABLED, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, GetModuleHandle(nullptr), nullptr));
	CHECK(m_hwnd);

	m_subclass = std::make_unique<WindowSubclass>(m_hwnd.get(),
		std::bind_front(&ShellChangeManager::WndProc, this));
}

ATOM ShellChangeManager::RegisterWindowClass()
{
	WNDCLASS windowClass = {};
	windowClass.lpfnWndProc = DefWindowProc;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.lpszClassName = CLASS_NAME;
	windowClass.hInstance = GetModuleHandle(nullptr);
	windowClass.style = 0;
	return RegisterClass(&windowClass);
}

LRESULT ShellChangeManager::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg >= MESSAGE_ID_MIN && msg <= MESSAGE_ID_MAX
		&& m_messageIdToWatchDetailsMap.contains(msg))
	{
		OnChangeNotify(msg, wParam, lParam);
	}

	switch (msg)
	{
	case WM_TIMER:
		if (wParam == PROCESS_CHANGES_TIMER_ID)
		{
			OnProcessChanges();
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void ShellChangeManager::OnChangeNotify(UINT msg, WPARAM wParam, LPARAM lParam)
{
	PIDLIST_ABSOLUTE *pidls;
	LONG event;
	HANDLE lock = SHChangeNotification_Lock(reinterpret_cast<HANDLE>(wParam),
		static_cast<DWORD>(lParam), &pidls, &event);

	m_changes.emplace_back(msg, event, pidls[0], pidls[1]);

	SHChangeNotification_Unlock(lock);

	SetTimer(m_hwnd.get(), PROCESS_CHANGES_TIMER_ID, PROCESS_CHANGES_TIMEOUT, nullptr);
}

void ShellChangeManager::OnProcessChanges()
{
	KillTimer(m_hwnd.get(), PROCESS_CHANGES_TIMER_ID);

	for (const auto &change : m_changes)
	{
		auto itr = m_messageIdToWatchDetailsMap.find(change.messageId);

		if (itr == m_messageIdToWatchDetailsMap.end())
		{
			continue;
		}

		const auto &watchDetails = itr->second;
		watchDetails.callback(change.event, change.pidl1, change.pidl2);
	}

	m_changes.clear();
}
