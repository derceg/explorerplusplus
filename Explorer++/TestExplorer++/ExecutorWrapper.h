// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <concurrencpp/concurrencpp.h>
#include <type_traits>

template <class T>
	requires std::is_base_of_v<concurrencpp::executor, T>
class ExecutorWrapper
{
public:
	ExecutorWrapper(std::shared_ptr<T> executor) : m_executor(executor)
	{
	}

	~ExecutorWrapper()
	{
		m_executor->shutdown();
	}

	std::shared_ptr<T> Get() const
	{
		return m_executor;
	}

private:
	const std::shared_ptr<T> m_executor;
};
