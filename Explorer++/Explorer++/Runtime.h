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
	Runtime(std::unique_ptr<concurrencpp::executor> uiThreadExecutor,
		std::unique_ptr<concurrencpp::executor> comStaExecutor);
	~Runtime();

	concurrencpp::executor *GetUiThreadExecutor() const;
	concurrencpp::executor *GetComStaExecutor() const;
	bool IsUiThread() const;

private:
	const std::unique_ptr<concurrencpp::executor> m_uiThreadExecutor;
	const std::unique_ptr<concurrencpp::executor> m_comStaExecutor;
	const UniqueThreadId m_uiThreadId;
};
