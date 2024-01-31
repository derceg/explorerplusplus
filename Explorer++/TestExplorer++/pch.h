// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#define STRICT

#define STRICT_TYPED_ITEMIDS

#define WIL_SUPPRESS_EXCEPTIONS

#include "../Helper/DisableUnaligned.h"

// Third-party Header Files:
#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Windows Header Files:
#include <Windows.h>
#include <windowsx.h>

// WinRT Header Files:
#include "../Helper/WinRTBaseWrapper.h"

// Boost Header Files:
#include <boost/bimap.hpp>
#include <boost/signals2.hpp>

// Google logging Header Files:
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>

// WIL Header Files:
#include <wil/resource.h>

// C++ Header Files:
#include <chrono>
#include <filesystem>
