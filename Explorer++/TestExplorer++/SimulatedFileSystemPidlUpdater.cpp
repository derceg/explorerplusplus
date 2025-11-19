// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SimulatedFileSystemPidlUpdater.h"
#include "SimulatedFileSystem.h"

SimulatedFileSystemPidlUpdater::SimulatedFileSystemPidlUpdater(SimulatedFileSystem *fileSystem) :
	m_fileSystem(fileSystem)
{
}

PidlAbsolute SimulatedFileSystemPidlUpdater::GetUpdatedPidl(const PidlAbsolute &pidl)
{
	return m_fileSystem->GetUpdatedPidl(pidl);
}
