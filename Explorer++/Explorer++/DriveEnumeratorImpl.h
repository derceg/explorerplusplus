// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DriveEnumerator.h"

class DriveEnumeratorImpl : public DriveEnumerator
{
public:
	// DriveEnumerator
	outcome::std_result<std::set<std::wstring>> GetDrives() override;
};
