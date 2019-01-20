#pragma once

#include "../Helper/PIDLWrapper.h"
#include "../Helper/ShellHelper.h"

struct ItemInfo_t
{
	PIDLPointer		pidlComplete;
	PIDLPointer		pridl;
	WIN32_FIND_DATA	wfd;
	TCHAR			szDisplayName[MAX_PATH];
	BOOL			bReal;
	BOOL			bIconRetrieved;
	int				iIcon;

	/* These are only used for drives. They are
	needed for when a drive is removed from the
	system, in which case the drive name is needed
	so that the removed drive can be found. */
	BOOL			bDrive;
	TCHAR			szDrive[4];

	/* Used for temporary sorting in details mode (i.e.
	when items need to be rearranged). */
	int				iRelativeSort;

	std::wstring getFullPath() const
	{
		TCHAR fullPath[MAX_PATH];
		GetDisplayName(pidlComplete.get(), fullPath, SIZEOF_ARRAY(fullPath), SHGDN_FORPARSING);
		return fullPath;
	}
};