// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FileOperations.h"
#include "MainResource.h"
#include "ResourceLoader.h"

namespace Epp
{

namespace FileOperations
{

HRESULT CopyFilesToFolder(HWND owner, std::vector<PCIDLIST_ABSOLUTE> &pidls, TransferAction action,
	const ResourceLoader *resourceLoader)
{
	auto title = resourceLoader->LoadString(IDS_GENERAL_COPY_TO_FOLDER_TITLE);
	return ::FileOperations::CopyFilesToFolder(owner, title, pidls, action);
}

}

}
