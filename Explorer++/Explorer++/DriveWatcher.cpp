// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DriveWatcher.h"

boost::signals2::connection DriveWatcher::AddDriveAddedObserver(
	const DriveAddedSignal::slot_type &observer)
{
	return m_driveAddedSignal.connect(observer);
}

boost::signals2::connection DriveWatcher::AddDriveUpdatedObserver(
	const DriveUpdatedSignal::slot_type &observer)
{
	return m_driveUpdatedSignal.connect(observer);
}

boost::signals2::connection DriveWatcher::AddDriveRemovedObserver(
	const DriveRemovedSignal::slot_type &observer)
{
	return m_driveRemovedSignal.connect(observer);
}
