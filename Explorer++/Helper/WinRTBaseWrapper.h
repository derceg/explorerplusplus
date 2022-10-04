// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

// It's important that Unknwn.h is included before any WinRT headers to ensure there is support for
// classic COM interfaces (i.e. IUnknown).
// clang-format off
#include <Unknwn.h>
#include <winrt/base.h>
// clang-format on
