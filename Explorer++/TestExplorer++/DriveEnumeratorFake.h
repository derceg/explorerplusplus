// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DriveEnumerator.h"

class DriveEnumeratorFake : public DriveEnumerator
{
public:
	DriveEnumeratorFake(const std::set<std::wstring> &drives);

	outcome::std_result<std::set<std::wstring>> GetDrives() override;

private:
	const std::set<std::wstring> m_drives;
};
