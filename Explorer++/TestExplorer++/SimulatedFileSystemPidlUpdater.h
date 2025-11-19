// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PidlUpdater.h"

class SimulatedFileSystem;

class SimulatedFileSystemPidlUpdater : public PidlUpdater
{
public:
	SimulatedFileSystemPidlUpdater(SimulatedFileSystem *fileSystem);

	PidlAbsolute GetUpdatedPidl(const PidlAbsolute &pidl) override;

private:
	SimulatedFileSystem *const m_fileSystem;
};
