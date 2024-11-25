// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Runtime.h"
#include <chrono>

using namespace std::chrono_literals;

Runtime::Runtime(std::shared_ptr<concurrencpp::executor> uiThreadExecutor,
	std::shared_ptr<concurrencpp::executor> comStaExecutor) :
	m_uiThreadExecutor(uiThreadExecutor),
	m_comStaExecutor(comStaExecutor),
	m_timerQueue(std::make_shared<concurrencpp::timer_queue>(120s)),
	m_uiThreadId(UniqueThreadId::GetForCurrentThread())
{
}

Runtime::~Runtime()
{
	m_uiThreadExecutor->shutdown();
	m_comStaExecutor->shutdown();
	m_timerQueue->shutdown();
}

std::shared_ptr<concurrencpp::executor> Runtime::GetUiThreadExecutor() const
{
	return m_uiThreadExecutor;
}

std::shared_ptr<concurrencpp::executor> Runtime::GetComStaExecutor() const
{
	return m_comStaExecutor;
}

std::shared_ptr<concurrencpp::timer_queue> Runtime::GetTimerQueue() const
{
	return m_timerQueue;
}

bool Runtime::IsUiThread() const
{
	return UniqueThreadId::GetForCurrentThread() == m_uiThreadId;
}
