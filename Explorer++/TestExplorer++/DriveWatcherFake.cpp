// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "DriveWatcherFake.h"

void DriveWatcherFake::AddDrive(const std::wstring &path)
{
	m_driveAddedSignal(path);
}

void DriveWatcherFake::UpdateDrive(const std::wstring &path)
{
	m_driveUpdatedSignal(path);
}

void DriveWatcherFake::RemoveDrive(const std::wstring &path)
{
	m_driveRemovedSignal(path);
}
