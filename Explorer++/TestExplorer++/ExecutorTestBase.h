// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <concurrencpp/concurrencpp.h>
#include <gtest/gtest.h>
#include <memory>

class ExecutorTestBase : public testing::Test
{
protected:
	ExecutorTestBase(std::shared_ptr<concurrencpp::executor> executor);
	~ExecutorTestBase();

	std::shared_ptr<concurrencpp::executor> m_executor;
};
