// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Handles the case where a file is dropped onto the
 * active listview.
 */

#include "stdafx.h"
#include "IDropFilesCallback.h"
#include "CoreInterface.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include <list>

DropFilesCallback::DropFilesCallback(CoreInterface *coreInterface) : m_coreInterface(coreInterface)
{
}

void DropFilesCallback::OnDropFile(const std::list<std::wstring> &PastedFileList, const POINT *ppt)
{
	UNREFERENCED_PARAMETER(ppt);

	std::vector<PidlAbsolute> pidls;

	for (const auto &pastedFile : PastedFileList)
	{
		unique_pidl_absolute pidl(SHSimpleIDListFromPath(pastedFile.c_str()));

		if (pidl)
		{
			pidls.push_back(pidl.get());
		}
	}

	m_coreInterface->GetActiveShellBrowserImpl()->SelectItems(pidls);
}
