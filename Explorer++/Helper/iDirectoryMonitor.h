#ifndef DIRECTORYMONITOR_INCLUDED
#define DIRECTORYMONITOR_ICLUDED

#include <windows.h>


/* Main exported interface. */
__interface IDirectoryMonitor : IUnknown
{
	int WatchDirectory(TCHAR *Directory,
		UINT WatchFlags,void (*DirectoryAltered)(TCHAR *szFileName,DWORD dwAction,void *pData),
		BOOL bWatchSubTree,void *pData);
	int WatchDirectory(HANDLE hDirectory,TCHAR *Directory,UINT WatchFlags,
		void (*DirectoryAltered)(TCHAR *szFileName,DWORD dwAction,void *pData),
		BOOL bWatchSubTree,void *pData);
	BOOL StopDirectoryMonitor(int iStopIndex);
};

HRESULT CreateDirectoryMonitor(IDirectoryMonitor **pDirectoryMonitor);

#endif