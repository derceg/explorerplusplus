// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "DriveEnumeratorFake.h"

DriveEnumeratorFake::DriveEnumeratorFake(const std::set<std::wstring> &drives) : m_drives(drives)
{
}

outcome::std_result<std::set<std::wstring>> DriveEnumeratorFake::GetDrives()
{
	return m_drives;
}
