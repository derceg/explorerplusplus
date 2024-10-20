// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Console.h"
#include <glog/logging.h>
#include <wil/resource.h>
#include <gdiplus.h>

using unique_gdiplus_shutdown = wil::unique_any<ULONG_PTR, decltype(&Gdiplus::GdiplusShutdown),
	Gdiplus::GdiplusShutdown, wil::details::pointer_access_none>;
using unique_glog_shutdown_call =
	wil::unique_call<decltype(&google::ShutdownGoogleLogging), google::ShutdownGoogleLogging>;
using unique_release_console_call =
	wil::unique_call<decltype(&Console::ReleaseConsole), Console::ReleaseConsole>;

[[nodiscard]] unique_gdiplus_shutdown CheckedGdiplusStartup();
[[nodiscard]] unique_glog_shutdown_call InitializeGoogleLogging();
[[nodiscard]] unique_release_console_call AttachParentConsole();
