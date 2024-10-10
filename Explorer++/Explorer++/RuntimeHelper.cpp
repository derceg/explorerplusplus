// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RuntimeHelper.h"
#include "Runtime.h"

[[nodiscard]] concurrencpp::lazy_result<void> ResumeOnUiThread(const Runtime *runtime)
{
	if (runtime->IsUiThread())
	{
		co_return;
	}

	co_await concurrencpp::resume_on(*runtime->GetUiThreadExecutor());
}

[[nodiscard]] concurrencpp::lazy_result<void> ResumeOnComStaThread(const Runtime *runtime)
{
	co_await concurrencpp::resume_on(*runtime->GetComStaExecutor());
}
