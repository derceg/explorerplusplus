// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "FolderSettings.h"
#include <boost/core/noncopyable.hpp>

class ShellBrowser;

struct PreservedFolderState : private boost::noncopyable
{
public:
	PreservedFolderState(const ShellBrowser *shellBrowser);

	FolderSettings folderSettings;
};
