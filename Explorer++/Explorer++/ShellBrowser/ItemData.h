#pragma once

#include "../Helper/PIDLWrapper.h"
#include "../Helper/ShellHelper.h"

struct BasicItemInfo_t
{
	PIDLPointer		pidlComplete;
	PIDLPointer		pridl;
	WIN32_FIND_DATA	wfd;
	TCHAR			szDisplayName[MAX_PATH];

	std::wstring getFullPath() const
	{
		TCHAR fullPath[MAX_PATH];
		GetDisplayName(pidlComplete.get(), fullPath, SIZEOF_ARRAY(fullPath), SHGDN_FORPARSING);
		return fullPath;
	}
};