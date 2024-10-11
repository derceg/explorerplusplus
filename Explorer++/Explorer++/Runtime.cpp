// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Runtime.h"

Runtime::Runtime(std::unique_ptr<concurrencpp::executor> uiThreadExecutor,
	std::unique_ptr<concurrencpp::executor> comStaExecutor) :
	m_uiThreadExecutor(std::move(uiThreadExecutor)),
	m_comStaExecutor(std::move(comStaExecutor)),
	m_uiThreadId(UniqueThreadId::GetForCurrentThread())
{
}

Runtime::~Runtime()
{
	m_uiThreadExecutor->shutdown();
	m_comStaExecutor->shutdown();
}

concurrencpp::executor *Runtime::GetUiThreadExecutor() const
{
	return m_uiThreadExecutor.get();
}

concurrencpp::executor *Runtime::GetComStaExecutor() const
{
	return m_comStaExecutor.get();
}

bool Runtime::IsUiThread() const
{
	return UniqueThreadId::GetForCurrentThread() == m_uiThreadId;
}
