#ifndef SHELLHELPER_INCLUDED
#define SHELLHELPER_INCLUDED

#include <list>

using namespace std;

#define CONTROL_PANEL_CATEGORY_VIEW	_T("::{26EE0668-A00A-44D7-9371-BEB064C98683}")

/* ---- Context menu handler registry entries ---- */

/* Defines the menu extensions shown on the background
context menu. */
#define CMH_DIRECTORY_BACKGROUND _T("Directory\\Background\\shellex\\ContextMenuHandlers")

/* Defines the menu extensions shown when dragging
and dropping (right-click). */
#define CMH_DRAGDROP_HANDLERS _T("Directory\\shellex\\DragDropHandlers")

struct JumpListTaskInformation
{
	const TCHAR	*pszName;
	const TCHAR	*pszPath;
	const TCHAR	*pszArguments;
	const TCHAR	*pszIconPath;
	int			iIcon;
};

struct ContextMenuHandler_t
{
	HMODULE		hDLL;
	IUnknown	*pUnknown;
};

void			DecodePath(TCHAR *szInitialPath,TCHAR *szCurrentDirectory,TCHAR *szParsingPath,size_t cchDest);
HRESULT			GetIdlFromParsingName(const TCHAR *szParsingName,LPITEMIDLIST *pidl);
HRESULT			GetDisplayName(TCHAR *szParsingPath,TCHAR *szDisplayName,DWORD uFlags);
HRESULT			GetDisplayName(LPITEMIDLIST pidlDirectory,TCHAR *szDisplayName,DWORD uFlags);
BOOL			CheckIdl(LPITEMIDLIST pidl);
BOOL			IsIdlDirectory(LPITEMIDLIST pidl);
void			GetVirtualFolderParsingPath(UINT uFolderCSIDL,TCHAR *szParsingPath);
HRESULT			GetVirtualParentPath(LPITEMIDLIST pidlDirectory,LPITEMIDLIST *pidlParent);
BOOL			IsNamespaceRoot(LPCITEMIDLIST pidl);
HRESULT			GetItemInfoTip(TCHAR *szItemPath,TCHAR *szInfoTip,int cchMax);
HRESULT			GetItemInfoTip(LPITEMIDLIST pidlComplete,TCHAR *szInfoTip,int cchMax);
HRESULT			GetCsidlFolderName(UINT csidl,TCHAR *szFolderName,DWORD uParsingFlags);
BOOL			MyExpandEnvironmentStrings(TCHAR *szSrc,TCHAR *szExpandedPath,DWORD nSize);
HRESULT			BuildHDropList(OUT FORMATETC *pftc,OUT STGMEDIUM *pstg,IN list<std::wstring> FilenameList);
HRESULT			BuildShellIDList(OUT FORMATETC *pftc,OUT STGMEDIUM *pstg,IN LPCITEMIDLIST pidlDirectory,IN list<LPITEMIDLIST> pidlList);
HRESULT			BindToShellFolder(LPCITEMIDLIST pidlDirectory,IShellFolder **pShellFolder);
BOOL			IsPathGUID(TCHAR *szPath);
BOOL			CompareIdls(LPCITEMIDLIST pidl1,LPCITEMIDLIST pidl2);
void			SetFORMATETC(FORMATETC *pftc,CLIPFORMAT cfFormat,DVTARGETDEVICE *ptd,DWORD dwAspect,LONG lindex,DWORD tymed);
HRESULT			AddJumpListTasks(std::list<JumpListTaskInformation> TaskList);
BOOL			LoadContextMenuHandlers(IN TCHAR *szRegKey,OUT list<ContextMenuHandler_t> *pContextMenuHandlers);
BOOL			LoadIUnknownFromCLSID(IN TCHAR *szCLSID,OUT ContextMenuHandler_t *pContextMenuHandler);

#endif