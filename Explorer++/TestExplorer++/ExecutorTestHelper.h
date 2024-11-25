// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <concurrencpp/concurrencpp.h>
#include <chrono>
#include <functional>
#include <memory>

static constexpr std::chrono::duration TASK_TIMEOUT_DURATION = std::chrono::seconds(1);

// Runs a task on the provided executor. Note that this function will block the current thread
// while it waits for the task to finish. Therefore, this shouldn't be used with any executor/task
// that requires the current thread to do ongoing work (e.g. pump messages).
void RunTaskOnExecutorForTest(std::shared_ptr<concurrencpp::executor> executor,
	std::function<void()> task);
