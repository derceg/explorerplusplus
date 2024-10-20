// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "UniqueResources.h"

unique_gdiplus_shutdown CheckedGdiplusStartup()
{
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::Status status = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
	CHECK_EQ(status, Gdiplus::Status::Ok);

	return unique_gdiplus_shutdown(gdiplusToken);
}

unique_glog_shutdown_call InitializeGoogleLogging()
{
	google::InitGoogleLogging(__argv[0]);
	return unique_glog_shutdown_call();
}
