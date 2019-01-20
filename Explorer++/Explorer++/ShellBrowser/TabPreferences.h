#pragma once

#include "../Helper/StringHelper.h"

struct Preferences_t
{
	BOOL showFriendlyDates;
	BOOL forceSize;
	SizeDisplayFormat_t sizeDisplayFormat;
	BOOL showFolderSizes;
	BOOL disableFolderSizesNetworkRemovable;
	BOOL hideLinkExtension;
	BOOL showExtensions;
};