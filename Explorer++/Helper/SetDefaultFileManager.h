#ifndef SETDEFAULTFILEMANAGER_INCLUDED
#define SETDEFAULTFILEMANAGER_INCLUDED

namespace NDefaultFileManager
{
	enum ReplaceExplorerModes_t
	{
		REPLACEEXPLORER_NONE		= 1,
		REPLACEEXPLORER_FILESYSTEM	= 2,
		REPLACEEXPLORER_ALL			= 3
	};

	BOOL	SetAsDefaultFileManagerFileSystem(TCHAR *szInternalCommand,TCHAR *szMenuText);
	BOOL	SetAsDefaultFileManagerAll(TCHAR *szInternalCommand,TCHAR *szMenuText);
	BOOL	RemoveAsDefaultFileManagerFileSystem(TCHAR *szInternalCommand);
	BOOL	RemoveAsDefaultFileManagerAll(TCHAR *szInternalCommand);
}

#endif