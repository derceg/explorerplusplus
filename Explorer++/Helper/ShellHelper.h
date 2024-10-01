// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PidlHelper.h"
#include <wil/resource.h>
#include <ShObjIdl.h>
#include <ShlGuid.h>
#include <exdisp.h>
#include <shellapi.h>
#include <list>
#include <optional>
#include <string>
#include <vector>

#define FRIENDLY_NAME_DESKTOP _T("Desktop")
#define FRIENDLY_NAME_PICTURES _T("Pictures")
#define FRIENDLY_NAME_MUSIC _T("Music")
#define FRIENDLY_NAME_VIDEOS _T("Videos")
#define FRIENDLY_NAME_DOCUMENTS _T("Documents")

// The path to the quick access folder on Windows 10.
static inline const WCHAR QUICK_ACCESS_PATH[] = L"shell:::{679f85cb-0220-4080-b29b-5540cc05aab6}";

// On Windows 11, the quick access folder is still present, but is effectively replaced with the
// home folder. The contents of the two folders are similar, but not identical. It's important to
// use this folder on Windows 11, since pinning/unpinning an item will generate change notifications
// in the home folder, not the quick access folder (despite the fact that the pinned items are
// displayed there as well).
static inline const WCHAR HOME_FOLDER_PATH[] = L"shell:::{f874310e-b6b7-47dc-bc84-b9e6b38f5903}";

// The path to the Linux distributions folder.
static inline const WCHAR WSL_DISTRIBUTIONS_PATH[] = L"\\\\wsl$";

/* See: http://msdn.microsoft.com/en-us/library/bb776902(v=VS.85).aspx#CFSTR_SHELLIDLIST */
#define HIDA_GetPIDLFolder(pida) (PCIDLIST_ABSOLUTE)(((LPBYTE) pida) + (pida)->aoffset[0])
#define HIDA_GetPIDLItem(pida, i) (PCIDLIST_RELATIVE)(((LPBYTE) pida) + (pida)->aoffset[i + 1])

const SHCOLUMNID SCID_ORIGINAL_LOCATION = { PSGUID_DISPLACED, PID_DISPLACED_FROM };
const SHCOLUMNID SCID_DATE_DELETED = { PSGUID_DISPLACED, PID_DISPLACED_DATE };

enum class EnvVarsExpansion
{
	Expand,
	DontExpand
};

enum class ShellItemType
{
	File,
	Folder
};

enum class LaunchCurrentProcessFlags
{
	None = 0,
	Elevated = 1 << 0
};

DEFINE_ENUM_FLAG_OPERATORS(LaunchCurrentProcessFlags);

struct JumpListTaskInformation
{
	const TCHAR *pszName;
	const TCHAR *pszPath;
	const TCHAR *pszArguments;
	const TCHAR *pszIconPath;
	int iIcon;
};

struct ContextMenuHandler
{
	HMODULE hDLL;
	IUnknown *pUnknown;
};

using unique_pidl_absolute = wil::unique_cotaskmem_ptr<std::remove_pointer_t<PIDLIST_ABSOLUTE>>;
using unique_pidl_relative = wil::unique_cotaskmem_ptr<std::remove_pointer_t<PIDLIST_RELATIVE>>;
using unique_pidl_child = wil::unique_cotaskmem_ptr<std::remove_pointer_t<PITEMID_CHILD>>;

using unique_shell_window_cookie = wil::unique_com_token<IShellWindows, long,
	decltype(&IShellWindows::Revoke), &IShellWindows::Revoke>;

HRESULT GetDisplayName(const std::wstring &parsingPath, DWORD flags, std::wstring &output);
HRESULT GetDisplayName(PCIDLIST_ABSOLUTE pidl, DWORD flags, std::wstring &output);
HRESULT GetDisplayName(IShellFolder *shellFolder, PCITEMID_CHILD pidlChild, DWORD flags,
	std::wstring &output);
HRESULT GetCsidlDisplayName(int csidl, DWORD flags, std::wstring &output);
HRESULT GetVirtualParentPath(PCIDLIST_ABSOLUTE pidlDirectory, PIDLIST_ABSOLUTE *pidlParent);
HRESULT GetRootPidl(PIDLIST_ABSOLUTE *pidl);
BOOL IsNamespaceRoot(PCIDLIST_ABSOLUTE pidl);
HRESULT BindToIdl(PCIDLIST_ABSOLUTE pidl, REFIID riid, void **ppv);
HRESULT GetUIObjectOf(IShellFolder *pShellFolder, HWND hwndOwner, UINT cidl,
	PCUITEMID_CHILD_ARRAY apidl, REFIID riid, void **ppv);
HRESULT ConvertVariantToString(const VARIANT *vt, TCHAR *szDetail, size_t cchMax,
	BOOL friendlyDate);
HRESULT ConvertVariantStringArrayToString(SAFEARRAY *array, TCHAR *szDetail, size_t cchMax);
HRESULT ConvertGenericVariantToString(const VARIANT *vt, TCHAR *szDetail, size_t cchMax);
HRESULT ConvertDateVariantToString(DATE date, TCHAR *szDetail, size_t cchMax, BOOL friendlyDate);
BOOL GetBooleanVariant(IShellFolder2 *shellFolder2, PCITEMID_CHILD pidlChild,
	const SHCOLUMNID *column, BOOL defaultValue);
std::wstring ConvertBstrToString(BSTR str);
std::optional<std::wstring> GetFolderPathForDisplay(PCIDLIST_ABSOLUTE pidl);
bool IsPathGUID(const std::wstring &path);
BOOL ArePidlsEquivalent(PCIDLIST_ABSOLUTE pidl1, PCIDLIST_ABSOLUTE pidl2);
HRESULT AddJumpListTasks(const std::list<JumpListTaskInformation> &taskList);
HRESULT GetItemAttributes(const TCHAR *szItemParsingPath, SFGAOF *pItemAttributes);
HRESULT GetItemAttributes(PCIDLIST_ABSOLUTE pidl, SFGAOF *pItemAttributes);
BOOL LaunchCurrentProcess(HWND hwnd, const std::wstring &parameters,
	LaunchCurrentProcessFlags flags = LaunchCurrentProcessFlags::None);
BOOL ExecuteFileAction(HWND hwnd, const std::wstring &itemPath, const std::wstring &verb,
	const std::wstring &parameters, const std::wstring &startDirectory);
BOOL ExecuteFileAction(HWND hwnd, PCIDLIST_ABSOLUTE pidl, const std::wstring &verb,
	const std::wstring &parameters, const std::wstring &startDirectory);
HRESULT DecodeFriendlyPath(const std::wstring &friendlyPath, std::wstring &parsingPath);
HRESULT ShowMultipleFileProperties(PCIDLIST_ABSOLUTE pidlDirectory,
	const std::vector<PCITEMID_CHILD> &items, HWND hwnd);
HRESULT ExecuteActionFromContextMenu(PCIDLIST_ABSOLUTE pidlDirectory,
	const std::vector<PCITEMID_CHILD> &items, HWND hwnd, const std::wstring &action, DWORD mask,
	IUnknown *site);
BOOL CompareVirtualFolders(const TCHAR *szDirectory, UINT uFolderCSIDL);
bool IsChildOfLibrariesFolder(PCIDLIST_ABSOLUTE pidl);
HRESULT CreateSimplePidl(const std::wstring &path, PidlAbsolute &outputPidl,
	IShellFolder *parent = nullptr, ShellItemType shellItemType = ShellItemType::File);
HRESULT UpdatePidl(PCIDLIST_ABSOLUTE inputPidl, PidlAbsolute &outputPidl);

std::optional<std::wstring> TransformUserEnteredPathToAbsolutePathAndNormalize(
	const std::wstring &userEnteredPath, const std::wstring &currentDirectory,
	EnvVarsExpansion envVarsExpansionType);
std::optional<std::wstring> MaybeExtractPathFromFileUrl(const std::wstring &url);
std::optional<std::wstring> ExpandEnvironmentStringsWrapper(const std::wstring &sourceString);
std::optional<std::wstring> MaybeTransformRootPathToAbsolutePath(const std::wstring path,
	const std::wstring &currentDirectory);
std::optional<std::wstring> PathCanonicalizeWrapper(const std::wstring &path);
std::optional<std::wstring> PathAppendWrapper(const std::wstring &path,
	const std::wstring &pathToAppend);
std::optional<std::wstring> PathStripToRootWrapper(const std::wstring &path);
std::optional<std::wstring> GetCurrentDirectoryWrapper();

/* Default icon indices. */
[[nodiscard]] HRESULT GetDefaultFolderIconIndex(int &outputImageIndex);
[[nodiscard]] HRESULT GetDefaultFileIconIndex(int &outputImageIndex);
[[nodiscard]] HRESULT GetDefaultIcon(SHSTOCKICONID iconId, int &outputImageIndex);

/* Infotips. */
HRESULT GetItemInfoTip(const std::wstring &itemPath, std::wstring &outputInfoTip);
HRESULT GetItemInfoTip(PCIDLIST_ABSOLUTE pidlComplete, std::wstring &outputInfoTip);

std::size_t hash_value(const IID &iid);

// Calling this function (rather than SHParseDisplayName() directly) ensures that search-ms: URLs
// will be treated as folders, which will allow navigations to those locations.
HRESULT ParseDisplayNameForNavigation(const std::wstring &itemPath, unique_pidl_absolute &pidlItem);

HRESULT MaybeGetLinkTarget(PCIDLIST_ABSOLUTE pidl, unique_pidl_absolute &targetPidl);
HRESULT MaybeResolveLinkTarget(HWND hwnd, PCIDLIST_ABSOLUTE pidl, unique_pidl_absolute &targetPidl);
