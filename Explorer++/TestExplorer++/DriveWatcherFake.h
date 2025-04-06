// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DriveWatcher.h"

class DriveWatcherFake : public DriveWatcher
{
public:
	void AddDrive(const std::wstring &path);
	void UpdateDrive(const std::wstring &path);
	void RemoveDrive(const std::wstring &path);
};
