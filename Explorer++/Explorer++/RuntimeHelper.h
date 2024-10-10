// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <concurrencpp/concurrencpp.h>

class Runtime;

[[nodiscard]] concurrencpp::lazy_result<void> ResumeOnUiThread(const Runtime *runtime);
[[nodiscard]] concurrencpp::lazy_result<void> ResumeOnComStaThread(const Runtime *runtime);
