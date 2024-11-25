// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/UniqueThreadId.h"
#include <boost/core/noncopyable.hpp>
#include <concurrencpp/concurrencpp.h>
#include <memory>

class Runtime : private boost::noncopyable
{
public:
	// Initializes the Runtime instance. This should be called from the UI thread.
	Runtime(std::shared_ptr<concurrencpp::executor> uiThreadExecutor,
		std::shared_ptr<concurrencpp::executor> comStaExecutor);
	~Runtime();

	std::shared_ptr<concurrencpp::executor> GetUiThreadExecutor() const;
	std::shared_ptr<concurrencpp::executor> GetComStaExecutor() const;
	bool IsUiThread() const;

private:
	const std::shared_ptr<concurrencpp::executor> m_uiThreadExecutor;
	const std::shared_ptr<concurrencpp::executor> m_comStaExecutor;
	const UniqueThreadId m_uiThreadId;
};
