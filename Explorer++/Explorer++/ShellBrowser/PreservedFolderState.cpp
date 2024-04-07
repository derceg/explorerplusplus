// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PreservedFolderState.h"
#include "ShellBrowserImpl.h"

PreservedFolderState::PreservedFolderState(const ShellBrowserImpl *shellBrowser) :
	folderSettings(shellBrowser->GetFolderSettings())
{
}
