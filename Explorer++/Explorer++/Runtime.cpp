// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Runtime.h"

Runtime::Runtime(std::shared_ptr<concurrencpp::executor> uiThreadExecutor,
	std::shared_ptr<concurrencpp::executor> comStaExecutor) :
	m_uiThreadExecutor(uiThreadExecutor),
	m_comStaExecutor(comStaExecutor),
	m_uiThreadId(UniqueThreadId::GetForCurrentThread())
{
}

Runtime::~Runtime()
{
	m_uiThreadExecutor->shutdown();
	m_comStaExecutor->shutdown();
}

std::shared_ptr<concurrencpp::executor> Runtime::GetUiThreadExecutor() const
{
	return m_uiThreadExecutor;
}

std::shared_ptr<concurrencpp::executor> Runtime::GetComStaExecutor() const
{
	return m_comStaExecutor;
}

bool Runtime::IsUiThread() const
{
	return UniqueThreadId::GetForCurrentThread() == m_uiThreadId;
}
