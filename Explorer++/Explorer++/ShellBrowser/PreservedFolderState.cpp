// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PreservedFolderState.h"
#include "ShellBrowser.h"

PreservedFolderState::PreservedFolderState(const ShellBrowser *shellBrowser) :
	folderSettings(shellBrowser->GetFolderSettings()),
	folderColumns(shellBrowser->GetAllColumnSets())
{
}
