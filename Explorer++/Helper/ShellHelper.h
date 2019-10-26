// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/optional.hpp>
#include <list>
#include <vector>

#define CONTROL_PANEL_CATEGORY_VIEW	_T("::{26EE0668-A00A-44D7-9371-BEB064C98683}")

#define FRIENDLY_NAME_DESKTOP	_T("Desktop")
#define FRIENDLY_NAME_PICTURES	_T("Pictures")
#define FRIENDLY_NAME_MUSIC		_T("Music")
#define FRIENDLY_NAME_VIDEOS	_T("Videos")
#define FRIENDLY_NAME_DOCUMENTS	_T("Documents")

/* See: http://msdn.microsoft.com/en-us/library/bb776902(v=VS.85).aspx#CFSTR_SHELLIDLIST */
#define HIDA_GetPIDLFolder(pida) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])
#define HIDA_GetPIDLItem(pida, i) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])

const SHCOLUMNID SCID_ORIGINAL_LOCATION = { PSGUID_DISPLACED, PID_DISPLACED_FROM };
const SHCOLUMNID SCID_DATE_DELETED = { PSGUID_DISPLACED, PID_DISPLACED_DATE };

enum DefaultIconType
{
	DEFAULT_ICON_FOLDER,
	DEFAULT_ICON_FILE
};

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

void			DecodePath(const TCHAR *szInitialPath,const TCHAR *szCurrentDirectory,TCHAR *szParsingPath,size_t cchDest);
HRESULT			GetIdlFromParsingName(const TCHAR *szParsingName,LPITEMIDLIST *pidl);
HRESULT			GetDisplayName(const TCHAR *szParsingPath,TCHAR *szDisplayName,UINT cchMax,DWORD uFlags);
HRESULT			GetDisplayName(LPCITEMIDLIST pidl,TCHAR *szDisplayName,UINT cchMax,DWORD uFlags);
HRESULT			GetCsidlDisplayName(int csidl, TCHAR *szFolderName, UINT cchMax, DWORD uParsingFlags);
BOOL			CheckIdl(LPCITEMIDLIST pidl);
BOOL			IsIdlDirectory(LPCITEMIDLIST pidl);
HRESULT			GetVirtualParentPath(LPITEMIDLIST pidlDirectory,LPITEMIDLIST *pidlParent);
BOOL			IsNamespaceRoot(LPCITEMIDLIST pidl);
BOOL			MyExpandEnvironmentStrings(const TCHAR *szSrc,TCHAR *szExpandedPath,DWORD nSize);
HRESULT			BuildHDropList(FORMATETC *pftc, STGMEDIUM *pstg, const std::list<std::wstring> &FilenameList);
HRESULT			BuildShellIDList(FORMATETC *pftc, STGMEDIUM *pstg, LPCITEMIDLIST pidlDirectory, const std::list<LPITEMIDLIST> &pidlList);
HRESULT			BindToIdl(LPCITEMIDLIST pidl, REFIID riid, void **ppv);
HRESULT			GetUIObjectOf(IShellFolder *pShellFolder, HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, void **ppv);
HRESULT			GetShellItemDetailsEx(IShellFolder2 *pShellFolder, const SHCOLUMNID *pscid, PCUITEMID_CHILD pidl, TCHAR *szDetail, size_t cchMax, BOOL friendlyDate);
HRESULT			ConvertVariantToString(const VARIANT *vt, TCHAR *szDetail, size_t cchMax, BOOL friendlyDate);
HRESULT			ConvertVariantStringArrayToString(SAFEARRAY *array, TCHAR *szDetail, size_t cchMax);
HRESULT			ConvertGenericVariantToString(const VARIANT *vt, TCHAR *szDetail, size_t cchMax);
HRESULT			ConvertDateVariantToString(DATE date, TCHAR *szDetail, size_t cchMax, BOOL friendlyDate);
boost::optional<std::wstring>	GetFolderPathForDisplay(LPCITEMIDLIST pidl);
BOOL			IsPathGUID(const TCHAR *szPath);
BOOL			CompareIdls(LPCITEMIDLIST pidl1,LPCITEMIDLIST pidl2);
HRESULT			AddJumpListTasks(const std::list<JumpListTaskInformation> &TaskList);
BOOL			LoadContextMenuHandlers(const TCHAR *szRegKey, std::list<ContextMenuHandler_t> &ContextMenuHandlers, const std::vector<std::wstring> &blacklistedCLSIDEntries);
BOOL			LoadIUnknownFromCLSID(const TCHAR *szCLSID, ContextMenuHandler_t *pContextMenuHandler);
HRESULT			GetItemAttributes(const TCHAR *szItemParsingPath, SFGAOF *pItemAttributes);
HRESULT			GetItemAttributes(LPCITEMIDLIST pidl, SFGAOF *pItemAttributes);
BOOL			ExecuteFileAction(HWND hwnd, const TCHAR *szVerb, const TCHAR *szParameters, const TCHAR *szStartDirectory, LPCITEMIDLIST pidl);
BOOL			ExecuteAndShowCurrentProcess(HWND hwnd, const TCHAR *szParameters);
BOOL			ExecuteAndShowProcess(HWND hwnd, const TCHAR *szProcess, const TCHAR *szParameters);
HRESULT			DecodeFriendlyPath(const TCHAR *szFriendlyPath,TCHAR *szParsingPath,UINT cchMax);
HRESULT			ShowMultipleFileProperties(LPITEMIDLIST pidlDirectory, LPCITEMIDLIST *ppidl, HWND hwndOwner, int nFiles);
HRESULT			ExecuteActionFromContextMenu(LPITEMIDLIST pidlDirectory, LPCITEMIDLIST *ppidl, HWND hwndOwner, int nFiles, const TCHAR *szAction, DWORD fMask);
BOOL			CompareVirtualFolders(const TCHAR *szDirectory, UINT uFolderCSIDL);
bool			IsChildOfLibrariesFolder(PIDLIST_ABSOLUTE pidl);

/* Drag and drop helpers. */
DWORD			DetermineDragEffect(DWORD grfKeyState, DWORD dwCurrentEffect, BOOL bDataAccept, BOOL bOnSameDrive);

/* Default icon indices. */
int				GetDefaultFolderIconIndex(void);
int				GetDefaultFileIconIndex(void);
int				GetDefaultIcon(DefaultIconType defaultIconType);

/* Infotips. */
HRESULT			GetItemInfoTip(const TCHAR *szItemPath, TCHAR *szInfoTip, size_t cchMax);
HRESULT			GetItemInfoTip(LPCITEMIDLIST pidlComplete, TCHAR *szInfoTip, size_t cchMax);