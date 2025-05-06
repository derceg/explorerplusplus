// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/FileOperations.h"

class ResourceLoader;

namespace Epp
{

namespace FileOperations
{

HRESULT CopyFilesToFolder(HWND owner, std::vector<PCIDLIST_ABSOLUTE> &pidls, TransferAction action,
	const ResourceLoader *resourceLoader);

}

}
