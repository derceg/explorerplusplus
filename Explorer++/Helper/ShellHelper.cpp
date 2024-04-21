// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellHelper.h"
#include "Helper.h"
#include "Macros.h"
#include "ProcessHelper.h"
#include "RegistrySettings.h"
#include "StringHelper.h"
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/container_hash/hash.hpp>
#include <glog/logging.h>
#include <wil/com.h>
#include <propkey.h>
#include <wininet.h>

enum class LinkTargetRetrievalType
{
	DontResolve,
	Resolve
};

BOOL ExecuteFileAction(HWND hwnd, const void *item, bool isPidl, const std::wstring &verb,
	const std::wstring &parameters, const std::wstring &startDirectory);

std::optional<std::wstring> TransformUserEnteredPathToAbsolutePath(
	const std::wstring &userEnteredPath, const std::wstring &currentDirectory,
	EnvVarsExpansion envVarsExpansionType);
bool ShouldNormalizePath(const std::wstring &path);

bool AddJumpListTasksInternal(IObjectCollection *objectCollection,
	const std::list<JumpListTaskInformation> &taskList);
HRESULT AddJumpListTaskInternal(IObjectCollection *objectCollection, const TCHAR *name,
	const TCHAR *path, const TCHAR *arguments, const TCHAR *iconPath, int iconIndex);

HRESULT MaybeGetLinkTarget(HWND hwnd, PCIDLIST_ABSOLUTE pidl, LinkTargetRetrievalType retrievalType,
	unique_pidl_absolute &targetPidl);

HRESULT GetDisplayName(const std::wstring &parsingPath, DWORD flags, std::wstring &output)
{
	unique_pidl_absolute pidl;
	HRESULT hr = SHParseDisplayName(parsingPath.c_str(), nullptr, wil::out_param(pidl), 0, nullptr);

	if (SUCCEEDED(hr))
	{
		hr = GetDisplayName(pidl.get(), flags, output);
	}

	return hr;
}

HRESULT GetDisplayName(PCIDLIST_ABSOLUTE pidl, DWORD flags, std::wstring &output)
{
	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	PCITEMID_CHILD pidlChild = nullptr;
	HRESULT hr = SHBindToParent(pidl, IID_PPV_ARGS(&shellFolder), &pidlChild);

	if (FAILED(hr))
	{
		return hr;
	}

	return GetDisplayName(shellFolder.get(), pidlChild, flags, output);
}

HRESULT GetDisplayName(IShellFolder *shellFolder, PCITEMID_CHILD pidlChild, DWORD flags,
	std::wstring &output)
{
	STRRET str;
	HRESULT hr = shellFolder->GetDisplayNameOf(pidlChild, flags, &str);

	if (FAILED(hr))
	{
		return hr;
	}

	wil::unique_cotaskmem_string name;
	hr = StrRetToStr(&str, pidlChild, &name);

	if (FAILED(hr))
	{
		return hr;
	}

	output = name.get();

	return hr;
}

HRESULT GetCsidlDisplayName(int csidl, DWORD flags, std::wstring &output)
{
	unique_pidl_absolute pidl;
	HRESULT hr = SHGetFolderLocation(nullptr, csidl, nullptr, 0, wil::out_param(pidl));

	if (SUCCEEDED(hr))
	{
		hr = GetDisplayName(pidl.get(), flags, output);
	}

	return hr;
}

HRESULT GetItemAttributes(const TCHAR *szItemParsingPath, SFGAOF *pItemAttributes)
{
	if (szItemParsingPath == nullptr || pItemAttributes == nullptr)
	{
		return E_FAIL;
	}

	unique_pidl_absolute pidl;
	HRESULT hr = SHParseDisplayName(szItemParsingPath, nullptr, wil::out_param(pidl), 0, nullptr);

	if (SUCCEEDED(hr))
	{
		hr = GetItemAttributes(pidl.get(), pItemAttributes);
	}

	return hr;
}

HRESULT GetItemAttributes(PCIDLIST_ABSOLUTE pidl, SFGAOF *pItemAttributes)
{
	if (pidl == nullptr || pItemAttributes == nullptr)
	{
		return E_FAIL;
	}

	IShellFolder *pShellFolder = nullptr;
	PCUITEMID_CHILD pidlRelative = nullptr;
	HRESULT hr = SHBindToParent(pidl, IID_PPV_ARGS(&pShellFolder), &pidlRelative);

	if (SUCCEEDED(hr))
	{
		hr = pShellFolder->GetAttributesOf(1, &pidlRelative, pItemAttributes);

		pShellFolder->Release();
	}

	return hr;
}

BOOL ExecuteFileAction(HWND hwnd, const std::wstring &itemPath, const std::wstring &verb,
	const std::wstring &parameters, const std::wstring &startDirectory)
{
	return ExecuteFileAction(hwnd, itemPath.c_str(), false, verb, parameters, startDirectory);
}

BOOL ExecuteFileAction(HWND hwnd, PCIDLIST_ABSOLUTE pidl, const std::wstring &verb,
	const std::wstring &parameters, const std::wstring &startDirectory)
{
	return ExecuteFileAction(hwnd, pidl, true, verb, parameters, startDirectory);
}

BOOL ExecuteFileAction(HWND hwnd, const void *item, bool isPidl, const std::wstring &verb,
	const std::wstring &parameters, const std::wstring &startDirectory)
{
	// Note that the SW_SHOWNORMAL display flag is used below. It's important to use that flag,
	// specifically, rather than something like SW_SHOW. That's because SW_SHOWNORMAL will ensure
	// that when a shortcut item is opened, the window display state set on the shortcut (i.e.
	// normal, minimized, maximized) will be correctly obeyed.
	SHELLEXECUTEINFO executeInfo = {};
	executeInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	executeInfo.fMask = SEE_MASK_DEFAULT;
	executeInfo.lpVerb = verb.empty() ? nullptr : verb.c_str();
	executeInfo.hwnd = hwnd;
	executeInfo.nShow = SW_SHOWNORMAL;
	executeInfo.lpParameters = parameters.empty() ? nullptr : parameters.c_str();
	executeInfo.lpDirectory = startDirectory.empty() ? nullptr : startDirectory.c_str();

	if (isPidl)
	{
		executeInfo.fMask |= SEE_MASK_INVOKEIDLIST;
		executeInfo.lpIDList = const_cast<void *>(item);
	}
	else
	{
		executeInfo.lpFile = static_cast<LPCWSTR>(item);
	}

	return ShellExecuteEx(&executeInfo);
}

BOOL ExecuteAndShowCurrentProcess(HWND hwnd, const TCHAR *szParameters)
{
	TCHAR szCurrentProcess[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), szCurrentProcess, SIZEOF_ARRAY(szCurrentProcess));

	return ExecuteAndShowProcess(hwnd, szCurrentProcess, szParameters);
}

BOOL ExecuteAndShowProcess(HWND hwnd, const TCHAR *szProcess, const TCHAR *szParameters)
{
	SHELLEXECUTEINFO sei;
	sei.cbSize = sizeof(sei);
	sei.fMask = SEE_MASK_DEFAULT;
	sei.lpVerb = _T("open");
	sei.lpFile = szProcess;
	sei.lpParameters = szParameters;
	sei.lpDirectory = nullptr;
	sei.hwnd = hwnd;
	sei.nShow = SW_SHOWNORMAL;
	return ShellExecuteEx(&sei);
}

HRESULT GetVirtualParentPath(PCIDLIST_ABSOLUTE pidlDirectory, PIDLIST_ABSOLUTE *pidlParent)
{
	if (IsNamespaceRoot(pidlDirectory))
	{
		*pidlParent = nullptr;
		return E_FAIL;
	}
	else
	{
		*pidlParent = ILCloneFull(pidlDirectory);
		ILRemoveLastID(*pidlParent);
		return S_OK;
	}
}

HRESULT GetRootPidl(PIDLIST_ABSOLUTE *pidl)
{
	// While using SHGetKnownFolderIDList() with FOLDERID_Desktop would be simpler than the method
	// used here, that method fails in Windows PE (with ERROR_FILE_NOT_FOUND). That failure is
	// unusual, since although the filesystem desktop folder doesn't exist, the virtual desktop
	// folder at the root of the shell namespace is still accessible and the pidl returned by
	// SHGetKnownFolderIDList() represents the root folder.
	// Retrieving the pidl using the method below works consistently, however.
	wil::com_ptr_nothrow<IShellFolder> desktop;
	RETURN_IF_FAILED(SHGetDesktopFolder(&desktop));
	return SHGetIDListFromObject(desktop.get(), pidl);
}

BOOL IsNamespaceRoot(PCIDLIST_ABSOLUTE pidl)
{
	// This method essentially just checks whether pidl->mkid.cb is 0. That will be the case for the
	// pidl representing the root desktop folder. Although the documentation doesn't appear to state
	// that, it can be confirmed by retrieving the root desktop pidl (pidl->mkid.cb will always be
	// 0).
	// The Wine test at
	// https://gitlab.winehq.org/wine/wine/-/blob/7ed17ec2511c85ce2e1f1fed0a9d04d85f658a5b/dlls/shell32/tests/shellpath.c#L2916
	// also provides evidence that an empty pidl represents the root desktop folder.
	return ILIsEmpty(pidl);
}

HRESULT DecodeFriendlyPath(const std::wstring &friendlyPath, std::wstring &parsingPath)
{
	PIDLIST_ABSOLUTE pidl = nullptr;
	std::wstring name;

	SHGetFolderLocation(nullptr, CSIDL_CONTROLS, nullptr, 0, &pidl);
	GetDisplayName(pidl, SHGDN_INFOLDER, name);
	CoTaskMemFree(pidl);

	if (name == friendlyPath)
	{
		GetCsidlDisplayName(CSIDL_CONTROLS, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	SHGetFolderLocation(nullptr, CSIDL_BITBUCKET, nullptr, 0, &pidl);
	GetDisplayName(pidl, SHGDN_INFOLDER, name);
	CoTaskMemFree(pidl);

	if (name == friendlyPath)
	{
		GetCsidlDisplayName(CSIDL_BITBUCKET, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	SHGetFolderLocation(nullptr, CSIDL_DRIVES, nullptr, 0, &pidl);
	GetDisplayName(pidl, SHGDN_INFOLDER, name);
	CoTaskMemFree(pidl);

	if (name == friendlyPath)
	{
		GetCsidlDisplayName(CSIDL_DRIVES, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	SHGetFolderLocation(nullptr, CSIDL_NETWORK, nullptr, 0, &pidl);
	GetDisplayName(pidl, SHGDN_INFOLDER, name);
	CoTaskMemFree(pidl);

	if (name == friendlyPath)
	{
		GetCsidlDisplayName(CSIDL_NETWORK, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	SHGetFolderLocation(nullptr, CSIDL_CONNECTIONS, nullptr, 0, &pidl);
	GetDisplayName(pidl, SHGDN_INFOLDER, name);
	CoTaskMemFree(pidl);

	if (name == friendlyPath)
	{
		GetCsidlDisplayName(CSIDL_CONNECTIONS, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	SHGetFolderLocation(nullptr, CSIDL_PRINTERS, nullptr, 0, &pidl);
	GetDisplayName(pidl, SHGDN_INFOLDER, name);
	CoTaskMemFree(pidl);

	if (name == friendlyPath)
	{
		GetCsidlDisplayName(CSIDL_PRINTERS, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	SHGetFolderLocation(nullptr, CSIDL_FAVORITES, nullptr, 0, &pidl);
	GetDisplayName(pidl, SHGDN_INFOLDER, name);
	CoTaskMemFree(pidl);

	if (name == friendlyPath)
	{
		GetCsidlDisplayName(CSIDL_FAVORITES, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	SHGetFolderLocation(nullptr, CSIDL_MYPICTURES, nullptr, 0, &pidl);
	GetDisplayName(pidl, SHGDN_INFOLDER, name);
	CoTaskMemFree(pidl);

	if (name == friendlyPath)
	{
		GetCsidlDisplayName(CSIDL_MYPICTURES, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	SHGetFolderLocation(nullptr, CSIDL_MYMUSIC, nullptr, 0, &pidl);
	GetDisplayName(pidl, SHGDN_INFOLDER, name);
	CoTaskMemFree(pidl);

	if (name == friendlyPath)
	{
		GetCsidlDisplayName(CSIDL_MYMUSIC, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	SHGetFolderLocation(nullptr, CSIDL_MYVIDEO, nullptr, 0, &pidl);
	GetDisplayName(pidl, SHGDN_INFOLDER, name);
	CoTaskMemFree(pidl);

	if (name == friendlyPath)
	{
		GetCsidlDisplayName(CSIDL_MYVIDEO, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, FRIENDLY_NAME_DESKTOP, -1,
			friendlyPath.c_str(), -1)
		== CSTR_EQUAL)
	{
		GetCsidlDisplayName(CSIDL_DESKTOP, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, FRIENDLY_NAME_PICTURES, -1,
			friendlyPath.c_str(), -1)
		== CSTR_EQUAL)
	{
		GetCsidlDisplayName(CSIDL_MYPICTURES, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, FRIENDLY_NAME_MUSIC, -1,
			friendlyPath.c_str(), -1)
		== CSTR_EQUAL)
	{
		GetCsidlDisplayName(CSIDL_MYMUSIC, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, FRIENDLY_NAME_VIDEOS, -1,
			friendlyPath.c_str(), -1)
		== CSTR_EQUAL)
	{
		GetCsidlDisplayName(CSIDL_MYVIDEO, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, FRIENDLY_NAME_DOCUMENTS, -1,
			friendlyPath.c_str(), -1)
		== CSTR_EQUAL)
	{
		GetCsidlDisplayName(CSIDL_MYDOCUMENTS, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	return E_FAIL;
}

HRESULT GetDefaultFolderIconIndex(int &outputImageIndex)
{
	return GetDefaultIcon(SIID_FOLDER, outputImageIndex);
}

HRESULT GetDefaultFileIconIndex(int &outputImageIndex)
{
	return GetDefaultIcon(SIID_DOCNOASSOC, outputImageIndex);
}

HRESULT GetDefaultIcon(SHSTOCKICONID iconId, int &outputImageIndex)
{
	SHSTOCKICONINFO info = {};
	info.cbSize = sizeof(info);
	RETURN_IF_FAILED(SHGetStockIconInfo(iconId, SHGSI_SYSICONINDEX, &info));

	outputImageIndex = info.iSysImageIndex;

	return S_OK;
}

HRESULT BindToIdl(PCIDLIST_ABSOLUTE pidl, REFIID riid, void **ppv)
{
	IShellFolder *pDesktop = nullptr;
	HRESULT hr = SHGetDesktopFolder(&pDesktop);

	if (SUCCEEDED(hr))
	{
		/* See http://blogs.msdn.com/b/oldnewthing/archive/2011/08/30/10202076.aspx. */
		if (pidl->mkid.cb)
		{
			hr = pDesktop->BindToObject(pidl, nullptr, riid, ppv);
		}
		else
		{
			hr = pDesktop->QueryInterface(riid, ppv);
		}

		pDesktop->Release();
	}

	return hr;
}

HRESULT GetUIObjectOf(IShellFolder *pShellFolder, HWND hwndOwner, UINT cidl,
	PCUITEMID_CHILD_ARRAY apidl, REFIID riid, void **ppv)
{
	return pShellFolder->GetUIObjectOf(hwndOwner, cidl, apidl, riid, nullptr, ppv);
}

HRESULT ConvertVariantToString(const VARIANT *vt, TCHAR *szDetail, size_t cchMax, BOOL friendlyDate)
{
	HRESULT hr;

	switch (vt->vt)
	{
	case VT_DATE:
		/* Although dates can be converted directly to strings, a custom
		conversion function is used for two reasons:
		1. To ensure that the date is in the local timezone (dates are
		in UTC by default).
		2. To ensure that the date display is consistent with how other
		dates in the application are displayed. */
		hr = ConvertDateVariantToString(vt->date, szDetail, cchMax, friendlyDate);
		break;

	case VT_ARRAY | VT_BSTR:
		hr = ConvertVariantStringArrayToString(vt->parray, szDetail, cchMax);
		break;

	default:
		hr = ConvertGenericVariantToString(vt, szDetail, cchMax);
		break;
	}

	return hr;
}

HRESULT ConvertDateVariantToString(DATE date, TCHAR *szDetail, size_t cchMax, BOOL friendlyDate)
{
	SYSTEMTIME systemTime;
	int variantConverted = VariantTimeToSystemTime(date, &systemTime);

	if (!variantConverted)
	{
		return E_FAIL;
	}

	SYSTEMTIME localSystemTime;
	BOOL systemTimeConverted =
		SystemTimeToTzSpecificLocalTime(nullptr, &systemTime, &localSystemTime);

	if (!systemTimeConverted)
	{
		return E_FAIL;
	}

	BOOL dateStringCreated =
		CreateSystemTimeString(&localSystemTime, szDetail, cchMax, friendlyDate);

	if (!dateStringCreated)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT ConvertVariantStringArrayToString(SAFEARRAY *array, TCHAR *szDetail, size_t cchMax)
{
	UINT dimensions = SafeArrayGetDim(array);

	if (dimensions != 1)
	{
		return E_INVALIDARG;
	}

	HRESULT hr;
	LONG lowerBound;
	hr = SafeArrayGetLBound(array, 1, &lowerBound);

	if (!SUCCEEDED(hr))
	{
		return hr;
	}

	LONG upperBound;
	hr = SafeArrayGetUBound(array, 1, &upperBound);

	if (!SUCCEEDED(hr))
	{
		return hr;
	}

	std::vector<std::wstring> strings;

	for (long i = lowerBound; i <= upperBound; i++)
	{
		BSTR element;
		hr = SafeArrayGetElement(array, &i, reinterpret_cast<void *>(&element));

		if (SUCCEEDED(hr))
		{
			strings.emplace_back(element);

			SysFreeString(element);
		}
	}

	std::wstring finalString = boost::algorithm::join(strings, L"; ");

	StringCchCopy(szDetail, cchMax, finalString.c_str());

	return S_OK;
}

HRESULT ConvertGenericVariantToString(const VARIANT *vt, TCHAR *szDetail, size_t cchMax)
{
	VARIANT vtDest;
	VariantInit(&vtDest);
	HRESULT hr = VariantChangeType(&vtDest, vt, 0, VT_BSTR);

	if (SUCCEEDED(hr))
	{
		hr = StringCchCopy(szDetail, cchMax, V_BSTR(&vtDest));
		VariantClear(&vtDest);
	}

	return hr;
}

BOOL GetBooleanVariant(IShellFolder2 *shellFolder2, PCITEMID_CHILD pidlChild,
	const SHCOLUMNID *column, BOOL defaultValue)
{
	VARIANT boolVariant;
	HRESULT hr = shellFolder2->GetDetailsEx(pidlChild, column, &boolVariant);

	if (FAILED(hr))
	{
		return defaultValue;
	}

	BOOL convertedValue;
	hr = VariantToBoolean(boolVariant, &convertedValue);

	if (FAILED(hr))
	{
		return defaultValue;
	}

	return convertedValue;
}

std::wstring ConvertBstrToString(BSTR str)
{
	if (str == nullptr)
	{
		return {};
	}

	return { str, SysStringLen(str) };
}

// Returns either the parsing path for the specified item, or its in
// folder name. The in folder name will be returned when the parsing
// path is a GUID (which typically shouldn't be displayed to the user).
std::optional<std::wstring> GetFolderPathForDisplay(PCIDLIST_ABSOLUTE pidl)
{
	std::wstring parsingPath;
	HRESULT hr = GetDisplayName(pidl, SHGDN_FORPARSING, parsingPath);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	if (IsPathGUID(parsingPath))
	{
		hr = GetDisplayName(pidl, SHGDN_INFOLDER, parsingPath);

		if (FAILED(hr))
		{
			return std::nullopt;
		}
	}

	return parsingPath;
}

// Returns true if a path is a GUID; i.e. of the form:
//
// ::{20D04FE0-3AEA-1069-A2D8-08002B30309D}
// (My Computer GUID, Windows 7)
bool IsPathGUID(const std::wstring &path)
{
	return path.starts_with(L"::");
}

// Note that this function doesn't currently support paths that cross from the file system to the
// shell namespace. For example, the path "c:\..\" refers to the computer folder. That folder,
// however, isn't part of the file system; instead it's part of the shell namespace.
std::optional<std::wstring> TransformUserEnteredPathToAbsolutePathAndNormalize(
	const std::wstring &userEnteredPath, const std::wstring &currentDirectory,
	EnvVarsExpansion envVarsExpansionType)
{
	auto absolutePath = TransformUserEnteredPathToAbsolutePath(userEnteredPath, currentDirectory,
		envVarsExpansionType);

	if (!absolutePath)
	{
		return std::nullopt;
	}

	if (!ShouldNormalizePath(*absolutePath))
	{
		return *absolutePath;
	}

	return PathCanonicalizeWrapper(*absolutePath);
}

std::optional<std::wstring> TransformUserEnteredPathToAbsolutePath(
	const std::wstring &userEnteredPath, const std::wstring &currentDirectory,
	EnvVarsExpansion envVarsExpansionType)
{
	std::wstring updatedPath = userEnteredPath;

	// An environment variable can appear anywhere in the path (regardless of what type of path has
	// been entered), so environment variables need to be expanded first. This means that it's also
	// not possible to interpret the path in any way until that expansion has taken place.
	if (envVarsExpansionType == EnvVarsExpansion::Expand)
	{
		auto expandedPath = ExpandEnvironmentStringsWrapper(updatedPath);

		if (expandedPath)
		{
			updatedPath = *expandedPath;
		}
	}

	// Trimming the path after expanding environment variables makes the most sense, since the
	// environment variables may themselves contain trailing spaces.
	boost::algorithm::trim_if(updatedPath,
		[](wchar_t character)
		{
			// As documented in
			// https://learn.microsoft.com/en-us/troubleshoot/windows-client/shell-experience/file-folder-name-whitespace-characters#summary,
			// leading and trailing whitespace will be removed from a file/folder name when saved.
			// In this context, only the ASCII space (0x20) counts as a whitespace character. So, it
			// should be safe to trim ASCII space characters here, but not any other whitespace
			// characters.
			return character == '\u0020';
		});

	if (updatedPath.empty())
	{
		return std::nullopt;
	}

	// The documentation for this method states that it performs rudimentary parsing. That should be
	// good enough, given that the goal here is simply to detect shell: and file: URLs.
	PARSEDURL parsedUrl = {};
	parsedUrl.cbSize = sizeof(parsedUrl);
	HRESULT hr = ParseURL(updatedPath.c_str(), &parsedUrl);

	if (SUCCEEDED(hr))
	{
		if (parsedUrl.nScheme == URL_SCHEME_SHELL)
		{
			// A shell: URL represents a shell folder path. For example, "shell:downloads" will
			// resolve to the user's downloads folder. These paths can be handled directly by
			// SHParseDisplayName().
			return updatedPath;
		}
		else if (parsedUrl.nScheme == URL_SCHEME_SEARCH_MS)
		{
			// This is a search-ms: URL.
			return updatedPath;
		}
		else if (parsedUrl.nScheme == URL_SCHEME_FILE)
		{
			// If the path is a file: URL, it should be absolute, meaning it can be returned
			// immediately. While Explorer doesn't normalize paths retrieved from file: URLs, it
			// appears that it is valid for a file: URL to contain relative references (see
			// https://datatracker.ietf.org/doc/html/rfc8089#appendix-E.2.1), so the path returned
			// here can still be normalized.
			return MaybeExtractPathFromFileUrl(updatedPath);
		}
		else
		{
			// Other types of URLs aren't supported.
			return std::nullopt;
		}
	}

	// '/' characters aren't valid filename characters, so it should be safe to transform them into
	// '\' characters. This allows paths like "c:/users" to be supported.
	std::replace(updatedPath.begin(), updatedPath.end(), '/', '\\');

	if (IsPathGUID(updatedPath) || !PathIsRelative(updatedPath.c_str()))
	{
		// Absolute paths can be returned unmodified, except for the case where a root path like
		// "\directory" is specified. In that case, the path is actually relative to the root of the
		// current directory.
		return MaybeTransformRootPathToAbsolutePath(updatedPath, currentDirectory);
	}

	return PathAppendWrapper(currentDirectory, updatedPath);
}

bool ShouldNormalizePath(const std::wstring &path)
{
	PARSEDURL parsedUrl = {};
	parsedUrl.cbSize = sizeof(parsedUrl);
	HRESULT hr = ParseURL(path.c_str(), &parsedUrl);

	// These URL types can't contain relative references.
	if (SUCCEEDED(hr)
		&& (parsedUrl.nScheme == URL_SCHEME_SHELL || parsedUrl.nScheme == URL_SCHEME_SEARCH_MS))
	{
		return false;
	}

	return true;
}

std::optional<std::wstring> MaybeExtractPathFromFileUrl(const std::wstring &url)
{
	if (url.size() >= INTERNET_MAX_URL_LENGTH)
	{
		return std::nullopt;
	}

	TCHAR path[MAX_PATH];
	DWORD size = SIZEOF_ARRAY(path);
	HRESULT hr = PathCreateFromUrl(url.c_str(), path, &size, 0);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	return path;
}

std::optional<std::wstring> ExpandEnvironmentStringsWrapper(const std::wstring &sourceString)
{
	auto length = ExpandEnvironmentStrings(sourceString.c_str(), nullptr, 0);

	if (length == 0)
	{
		return std::nullopt;
	}

	std::wstring expandedString;
	expandedString.resize(length);

	length = ExpandEnvironmentStrings(sourceString.c_str(), expandedString.data(), length);

	if (length == 0)
	{
		return std::nullopt;
	}

	// length includes the terminating NULL character, which shouldn't be included in the actual
	// string.
	expandedString.resize(length - 1);

	return expandedString;
}

// Transforms a path of the form "\directory" (referred to in the documentation as an absolute path,
// but here as a root path, since it's relative to the root of the current directory) into a full
// path (e.g. "c:\directory"). If the supplied path isn't in that form, it will simply be returned
// without modification.
std::optional<std::wstring> MaybeTransformRootPathToAbsolutePath(const std::wstring path,
	const std::wstring &currentDirectory)
{
	// As described in
	// https://learn.microsoft.com/en-au/windows/win32/fileio/naming-a-file#fully-qualified-vs-relative-paths,
	// a path like "\directory" is an absolute path. It refers to an item under the root of the
	// current directory. For example, with the following:
	//
	// Current directory: c:\users\public
	// Path: \windows
	//
	// The resulting path should be "c:\windows".
	if ((path.size() == 1 && path[0] == '\\')
		|| (path.size() >= 2 && path[0] == '\\' && path[1] != '\\'))
	{
		auto root = PathStripToRootWrapper(currentDirectory);

		if (!root)
		{
			return std::nullopt;
		}

		if (!root->ends_with('\\'))
		{
			*root += '\\';
		}

		return *root + path.substr(1);
	}

	return path;
}

std::optional<std::wstring> PathCanonicalizeWrapper(const std::wstring &path)
{
	// The input path passed to PathCanonicalize can't be longer than MAX_PATH.
	if (path.size() >= MAX_PATH)
	{
		return std::nullopt;
	}

	// Ideally, it would be better to use std::filesystem to normalize the path, but that has some
	// unusual behavior in how it treats paths that use the long path prefix (also see
	// https://github.com/microsoft/STL/issues/2256). For example:
	//
	// std::filesystem::path("\\\\?\\c:\\..\\").lexically_normal() == "\\\\?\\"
	//
	// whereas PathCanonicalize produces "c:\\".
	// TODO: If supporting only Windows 8 and above, it would make more sense to use
	// PathCchCanonicalize or PathCchCanonicalizeEx.
	TCHAR normalizedPath[MAX_PATH];
	BOOL res = PathCanonicalize(normalizedPath, path.c_str());

	if (!res)
	{
		return std::nullopt;
	}

	return normalizedPath;
}

std::optional<std::wstring> PathAppendWrapper(const std::wstring &path,
	const std::wstring &pathToAppend)
{
	if (path.size() >= MAX_PATH || pathToAppend.size() >= MAX_PATH)
	{
		return std::nullopt;
	}

	TCHAR finalPath[MAX_PATH];
	StringCchCopy(finalPath, SIZEOF_ARRAY(finalPath), path.c_str());
	BOOL res = PathAppend(finalPath, pathToAppend.c_str());

	if (!res)
	{
		return std::nullopt;
	}

	return finalPath;
}

std::optional<std::wstring> PathStripToRootWrapper(const std::wstring &path)
{
	if (path.size() >= MAX_PATH)
	{
		return std::nullopt;
	}

	TCHAR root[MAX_PATH];
	StringCchCopy(root, SIZEOF_ARRAY(root), path.c_str());
	BOOL res = PathStripToRoot(root);

	if (!res)
	{
		return std::nullopt;
	}

	return root;
}

std::optional<std::wstring> GetCurrentDirectoryWrapper()
{
	auto length = GetCurrentDirectory(0, nullptr);

	if (length == 0)
	{
		return std::nullopt;
	}

	std::wstring currentDirectory;
	currentDirectory.resize(length);

	length = GetCurrentDirectory(length, currentDirectory.data());

	if (length == 0)
	{
		return std::nullopt;
	}

	// When requesting the length of the buffer, the return value includes the terminating NULL
	// character. When the function succeeds, the return value doesn't include the terminating NULL
	// character. So, resizing to length here simply excludes the terminating NULL character from
	// the string.
	currentDirectory.resize(length);

	return currentDirectory;
}

BOOL ArePidlsEquivalent(PCIDLIST_ABSOLUTE pidl1, PCIDLIST_ABSOLUTE pidl2)
{
	IShellFolder *pDesktopFolder = nullptr;
	HRESULT hr;
	BOOL ret = FALSE;

	hr = SHGetDesktopFolder(&pDesktopFolder);

	if (SUCCEEDED(hr))
	{
		hr = pDesktopFolder->CompareIDs(SHCIDS_CANONICALONLY, pidl1, pidl2);

		if (HRESULT_CODE(hr) == 0)
		{
			ret = TRUE;
		}

		pDesktopFolder->Release();
	}

	return ret;
}

HRESULT AddJumpListTasks(const std::list<JumpListTaskInformation> &taskList)
{
	wil::com_ptr_nothrow<ICustomDestinationList> customDestinationList;
	HRESULT hr = CoCreateInstance(CLSID_DestinationList, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&customDestinationList));

	if (FAILED(hr))
	{
		return hr;
	}

	wil::com_ptr_nothrow<IObjectArray> removedItems;
	UINT minSlots;
	hr = customDestinationList->BeginList(&minSlots, IID_PPV_ARGS(&removedItems));

	if (FAILED(hr))
	{
		return hr;
	}

	wil::com_ptr_nothrow<IObjectCollection> objectCollection;
	hr = CoCreateInstance(CLSID_EnumerableObjectCollection, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&objectCollection));

	if (FAILED(hr))
	{
		return hr;
	}

	AddJumpListTasksInternal(objectCollection.get(), taskList);

	wil::com_ptr_nothrow<IObjectArray> items;
	hr = objectCollection->QueryInterface(IID_PPV_ARGS(&items));

	if (FAILED(hr))
	{
		return hr;
	}

	hr = customDestinationList->AddUserTasks(items.get());

	if (FAILED(hr))
	{
		return hr;
	}

	hr = customDestinationList->CommitList();

	return hr;
}

bool AddJumpListTasksInternal(IObjectCollection *objectCollection,
	const std::list<JumpListTaskInformation> &taskList)
{
	bool allSucceeded = true;

	for (const auto &jtli : taskList)
	{
		HRESULT hr = AddJumpListTaskInternal(objectCollection, jtli.pszName, jtli.pszPath,
			jtli.pszArguments, jtli.pszIconPath, jtli.iIcon);

		if (FAILED(hr))
		{
			allSucceeded = false;
		}
	}

	return allSucceeded;
}

HRESULT AddJumpListTaskInternal(IObjectCollection *objectCollection, const TCHAR *name,
	const TCHAR *path, const TCHAR *arguments, const TCHAR *iconPath, int iconIndex)
{
	wil::com_ptr_nothrow<IShellLink> shellLink;
	HRESULT hr =
		CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink));

	if (FAILED(hr))
	{
		return hr;
	}

	shellLink->SetPath(path);
	shellLink->SetArguments(arguments);
	shellLink->SetIconLocation(iconPath, iconIndex);

	wil::com_ptr_nothrow<IPropertyStore> propertyStore;
	hr = shellLink->QueryInterface(IID_PPV_ARGS(&propertyStore));

	if (FAILED(hr))
	{
		return hr;
	}

	wil::unique_prop_variant titleProperty;
	hr = InitPropVariantFromString(name, &titleProperty);

	if (FAILED(hr))
	{
		return hr;
	}

	hr = propertyStore->SetValue(PKEY_Title, titleProperty);

	if (FAILED(hr))
	{
		return hr;
	}

	hr = propertyStore->Commit();

	if (FAILED(hr))
	{
		return hr;
	}

	hr = objectCollection->AddObject(shellLink.get());

	return hr;
}

/* Returns a list of DLL's/IUnknown interfaces. Note that
is up to the caller to free both the DLL's and objects
returned.

http://www.ureader.com/msg/16601280.aspx

Also note that a set of blacklisted CLSID entries can be
provided. Any entries in this set will be ignored (i.e. they
won't be loaded). Each entry should be a CLSID with the enclosing
braces included. */
BOOL LoadContextMenuHandlers(const TCHAR *szRegKey,
	std::list<ContextMenuHandler> &contextMenuHandlers,
	const std::vector<std::wstring> &blacklistedCLSIDEntries)
{
	HKEY hKey = nullptr;
	BOOL bSuccess = FALSE;

	LONG lRes = RegOpenKeyEx(HKEY_CLASSES_ROOT, szRegKey, 0, KEY_READ, &hKey);

	if (lRes == ERROR_SUCCESS)
	{
		TCHAR szKeyName[512];
		int iIndex = 0;

		DWORD dwLen = SIZEOF_ARRAY(szKeyName);

		while ((lRes = RegEnumKeyEx(hKey, iIndex, szKeyName, &dwLen, nullptr, nullptr, nullptr,
					nullptr))
			== ERROR_SUCCESS)
		{
			HKEY hSubKey;
			TCHAR szSubKey[512];
			LONG lSubKeyRes;

			StringCchPrintf(szSubKey, SIZEOF_ARRAY(szSubKey), _T("%s\\%s"), szRegKey, szKeyName);

			lSubKeyRes = RegOpenKeyEx(HKEY_CLASSES_ROOT, szSubKey, 0, KEY_READ, &hSubKey);

			if (lSubKeyRes == ERROR_SUCCESS)
			{
				std::wstring clsid;
				LSTATUS clsidRes = RegistrySettings::ReadString(hSubKey, L"", clsid);

				if (clsidRes == ERROR_SUCCESS)
				{
					if (std::none_of(blacklistedCLSIDEntries.begin(), blacklistedCLSIDEntries.end(),
							[&clsid](const std::wstring &blacklistedEntry)
							{ return boost::iequals(clsid, blacklistedEntry); }))
					{
						ContextMenuHandler contextMenuHandler;

						BOOL bRes = LoadIUnknownFromCLSID(clsid.c_str(), &contextMenuHandler);

						if (bRes)
						{
							contextMenuHandlers.push_back(contextMenuHandler);
						}
					}
				}

				RegCloseKey(hSubKey);
			}

			dwLen = SIZEOF_ARRAY(szKeyName);
			iIndex++;
		}

		RegCloseKey(hKey);

		bSuccess = TRUE;
	}

	return bSuccess;
}

/* Extracts an IUnknown interface from a class object,
based on its CLSID. If the CLSID exists in
HKLM\Software\Classes\CLSID, the DLL for this object
will attempted to be loaded.
Regardless of whether or not a DLL was actually
loaded, the object will be initialized with a call
to CoCreateInstance. */
BOOL LoadIUnknownFromCLSID(const TCHAR *szCLSID, ContextMenuHandler *pContextMenuHandler)
{
	HKEY hCLSIDKey;
	HKEY hDllKey;
	HMODULE hDLL = nullptr;
	TCHAR szCLSIDKey[512];
	LONG lRes;
	BOOL bSuccess = FALSE;

	StringCchPrintf(szCLSIDKey, SIZEOF_ARRAY(szCLSIDKey), _T("%s\\%s"),
		_T("Software\\Classes\\CLSID"), szCLSID);

	/* Open the CLSID key. */
	lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szCLSIDKey, 0, KEY_READ, &hCLSIDKey);

	if (lRes == ERROR_SUCCESS)
	{
		lRes = RegOpenKeyEx(hCLSIDKey, _T("InProcServer32"), 0, KEY_READ, &hDllKey);

		if (lRes == ERROR_SUCCESS)
		{
			std::wstring dll;
			LSTATUS dllRes = RegistrySettings::ReadString(hDllKey, L"", dll);

			if (dllRes == ERROR_SUCCESS)
			{
				/* Now, load the DLL it refers to. */
				hDLL = LoadLibrary(dll.c_str());
			}

			RegCloseKey(hDllKey);
		}

		RegCloseKey(hCLSIDKey);
	}

	CLSID clsid;

	/* Regardless of whether or not any DLL was
	loaded, attempt to create the object. */
	HRESULT hr = CLSIDFromString(szCLSID, &clsid);

	if (hr == NO_ERROR)
	{
		IUnknown *pUnknown = nullptr;

		hr = CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pUnknown));

		if (hr == S_OK)
		{
			bSuccess = TRUE;

			pContextMenuHandler->hDLL = hDLL;
			pContextMenuHandler->pUnknown = pUnknown;
		}
	}

	if (!bSuccess)
	{
		if (hDLL != nullptr)
		{
			FreeLibrary(hDLL);
		}
	}

	return bSuccess;
}

HRESULT GetItemInfoTip(const std::wstring &itemPath, std::wstring &outputInfoTip)
{
	unique_pidl_absolute pidlItem;
	RETURN_IF_FAILED(
		SHParseDisplayName(itemPath.c_str(), nullptr, wil::out_param(pidlItem), 0, nullptr));

	return GetItemInfoTip(pidlItem.get(), outputInfoTip);
}

HRESULT GetItemInfoTip(PCIDLIST_ABSOLUTE pidlComplete, std::wstring &outputInfoTip)
{
	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	PCITEMID_CHILD pidlRelative;
	RETURN_IF_FAILED(SHBindToParent(pidlComplete, IID_PPV_ARGS(&shellFolder), &pidlRelative));

	wil::com_ptr_nothrow<IQueryInfo> queryInfo;
	RETURN_IF_FAILED(
		GetUIObjectOf(shellFolder.get(), nullptr, 1, &pidlRelative, IID_PPV_ARGS(&queryInfo)));

	wil::unique_cotaskmem_string infoTip;
	RETURN_IF_FAILED(queryInfo->GetInfoTip(QITIPF_USESLOWTIP, &infoTip));

	if (infoTip)
	{
		outputInfoTip = infoTip.get();
	}
	else
	{
		outputInfoTip.clear();
	}

	return S_OK;
}

HRESULT ShowMultipleFileProperties(PCIDLIST_ABSOLUTE pidlDirectory,
	const std::vector<PCITEMID_CHILD> &items, HWND hwnd)
{
	return ExecuteActionFromContextMenu(pidlDirectory, items, hwnd, _T("properties"), 0, nullptr);
}

HRESULT ExecuteActionFromContextMenu(PCIDLIST_ABSOLUTE pidlDirectory,
	const std::vector<PCITEMID_CHILD> &items, HWND hwnd, const std::wstring &action, DWORD mask,
	IUnknown *site)
{
	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	RETURN_IF_FAILED(BindToIdl(pidlDirectory, IID_PPV_ARGS(&shellFolder)));

	wil::com_ptr_nothrow<IContextMenu> contextMenu;

	if (items.empty())
	{
		RETURN_IF_FAILED(shellFolder->CreateViewObject(hwnd, IID_PPV_ARGS(&contextMenu)));
	}
	else
	{
		RETURN_IF_FAILED(GetUIObjectOf(shellFolder.get(), hwnd, static_cast<UINT>(items.size()),
			items.data(), IID_PPV_ARGS(&contextMenu)));
	}

	if (site)
	{
		// The IObjectWithSite interface may not be available in some cases - for example, when the
		// directory is a .zip file. So it's not safe to assume that this will always succeed.
		wil::com_ptr_nothrow<IObjectWithSite> objectWithSite;
		HRESULT hr = contextMenu->QueryInterface(IID_PPV_ARGS(&objectWithSite));

		if (SUCCEEDED(hr))
		{
			// The site here is used to select items after they've been pasted. If SetSite() fails,
			// it would still be useful to perform the paste operation anyway. It's not expected
			// that the call would actually fail, but if it does, it would be useful to know, hence
			// the assert.
			hr = objectWithSite->SetSite(site);
			assert(SUCCEEDED(hr));
		}
	}

	auto actionNarrow = WstrToStr(action);

	if (!actionNarrow)
	{
		return E_FAIL;
	}

	CMINVOKECOMMANDINFO commandInfo;
	commandInfo.cbSize = sizeof(CMINVOKECOMMANDINFO);
	commandInfo.fMask = mask;
	commandInfo.hwnd = hwnd;
	commandInfo.lpVerb = actionNarrow->c_str();
	commandInfo.lpParameters = nullptr;
	commandInfo.lpDirectory = nullptr;
	commandInfo.nShow = SW_SHOWNORMAL;
	RETURN_IF_FAILED(contextMenu->InvokeCommand(&commandInfo));

	return S_OK;
}

BOOL CompareVirtualFolders(const TCHAR *szDirectory, UINT uFolderCSIDL)
{
	std::wstring parsingPath;
	GetCsidlDisplayName(uFolderCSIDL, SHGDN_FORPARSING, parsingPath);

	if (parsingPath == szDirectory)
	{
		return TRUE;
	}

	return FALSE;
}

bool IsChildOfLibrariesFolder(PCIDLIST_ABSOLUTE pidl)
{
	unique_pidl_absolute pidlLibraries;
	HRESULT hr =
		SHGetKnownFolderIDList(FOLDERID_Libraries, 0, nullptr, wil::out_param(pidlLibraries));

	if (FAILED(hr))
	{
		return false;
	}

	return ILIsParent(pidlLibraries.get(), pidl, FALSE);
}

class FileSystemBindData : public IFileSystemBindData
{
public:
	static wil::com_ptr_nothrow<FileSystemBindData> Create(const WIN32_FIND_DATA *wfd)
	{
		wil::com_ptr_nothrow<FileSystemBindData> fsBindData;
		fsBindData.attach(new FileSystemBindData(wfd));
		return fsBindData;
	}

	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppvObject)
	{
		// clang-format off
		static const QITAB qit[] = {
			QITABENT(FileSystemBindData, IFileSystemBindData),
			{ nullptr }
		};
		// clang-format on

		return QISearch(this, qit, riid, ppvObject);
	}

	IFACEMETHODIMP_(ULONG) AddRef(void)
	{
		return InterlockedIncrement(&m_refCount);
	}

	IFACEMETHODIMP_(ULONG) Release(void)
	{
		ULONG refCount = InterlockedDecrement(&m_refCount);

		if (refCount == 0)
		{
			delete this;
		}

		return refCount;
	}

	IFACEMETHODIMP SetFindData(const WIN32_FIND_DATAW *wfd)
	{
		m_wfd = *wfd;
		return S_OK;
	}

	IFACEMETHODIMP GetFindData(WIN32_FIND_DATAW *wfd)
	{
		*wfd = m_wfd;
		return S_OK;
	}

private:
	ULONG m_refCount;
	WIN32_FIND_DATA m_wfd;

	FileSystemBindData(const WIN32_FIND_DATA *wfd) : m_refCount(1), m_wfd(*wfd)
	{
	}
};

// This performs the same function as SHSimpleIDListFromPath(), which is deprecated.
// The path provided should be relative to the parent. If parent is null, the path should be
// absolute.
HRESULT CreateSimplePidl(const std::wstring &path, PidlAbsolute &outputPidl, IShellFolder *parent,
	ShellItemType shellItemType)
{
	wil::com_ptr_nothrow<IBindCtx> bindCtx;
	RETURN_IF_FAILED(CreateBindCtx(0, &bindCtx));

	BIND_OPTS opts = { sizeof(opts), 0, STGM_CREATE, 0 };
	RETURN_IF_FAILED(bindCtx->SetBindOptions(&opts));

	WIN32_FIND_DATA wfd = {};

	switch (shellItemType)
	{
	case ShellItemType::File:
		wfd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
		break;

	case ShellItemType::Folder:
		wfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		break;
	}

	auto fsBindData = FileSystemBindData::Create(&wfd);

	RETURN_IF_FAILED(
		bindCtx->RegisterObjectParam(const_cast<PWSTR>(STR_FILE_SYS_BIND_DATA), fsBindData.get()));

	if (!parent)
	{
		return SHParseDisplayName(path.c_str(), bindCtx.get(), PidlOutParam(outputPidl), 0,
			nullptr);
	}

	unique_pidl_relative pidlRelative;
	RETURN_IF_FAILED(parent->ParseDisplayName(nullptr, bindCtx.get(),
		const_cast<LPWSTR>(path.c_str()), nullptr, wil::out_param(pidlRelative), nullptr));

	unique_pidl_absolute pidlParent;
	RETURN_IF_FAILED(SHGetIDListFromObject(parent, wil::out_param(pidlParent)));

	outputPidl.TakeOwnership(ILCombine(pidlParent.get(), pidlRelative.get()));

	return S_OK;
}

// This performs the same function as SHGetRealIDL, which is deprecated.
HRESULT SimplePidlToFullPidl(PCIDLIST_ABSOLUTE simplePidl, PIDLIST_ABSOLUTE *fullPidl)
{
	wil::com_ptr_nothrow<IShellItem2> shellItem2;
	RETURN_IF_FAILED(SHCreateItemFromIDList(simplePidl, IID_PPV_ARGS(&shellItem2)));
	RETURN_IF_FAILED(shellItem2->Update(nullptr));

	wil::com_ptr_nothrow<IParentAndItem> parentAndItem;
	RETURN_IF_FAILED(shellItem2->QueryInterface(IID_PPV_ARGS(&parentAndItem)));

	unique_pidl_absolute parent;
	unique_pidl_child child;
	RETURN_IF_FAILED(
		parentAndItem->GetParentAndItem(wil::out_param(parent), nullptr, wil::out_param(child)));

	*fullPidl = ILCombine(parent.get(), child.get());

	return S_OK;
}

std::size_t hash_value(const IID &iid)
{
	std::size_t seed = 0;
	boost::hash_combine(seed, iid.Data1);
	boost::hash_combine(seed, iid.Data2);
	boost::hash_combine(seed, iid.Data3);
	boost::hash_combine(seed, iid.Data4);
	return seed;
}

// When calling IBindCtx::RegisterObjectParam(), a valid COM object instance needs to be provided,
// even if it's not actually used to carry any data. Therefore, this class exists purely for that
// purpose.
class DummyUnknown : public winrt::implements<DummyUnknown, IUnknown, winrt::non_agile>
{
};

HRESULT CreateBindCtxWithParam(const std::wstring &param,
	wil::com_ptr_nothrow<IBindCtx> &outputBindCtx)
{
	wil::com_ptr_nothrow<IBindCtx> bindCtx;
	RETURN_IF_FAILED(CreateBindCtx(0, &bindCtx));

	// The second parameter here needs to be a valid pointer to a COM object, even though the
	// parameter isn't actually used.
	auto dummyUnknown = winrt::make_self<DummyUnknown>();
	RETURN_IF_FAILED(
		bindCtx->RegisterObjectParam(const_cast<PWSTR>(param.c_str()), dummyUnknown.get()));

	outputBindCtx = bindCtx;

	return S_OK;
}

HRESULT ParseDisplayNameForNavigation(const std::wstring &itemPath, unique_pidl_absolute &pidlItem)
{
	// Using this ensures that a search-ms: URL will be treated as a folder. Without this,
	// attempting to enumerate the items associated with a search-ms: URL will fail.
	// This also appears to impact the pidl returned for certain paths.
	// https://explorerplusplus.com/forum/viewtopic.php?t=3185 describes an issue in which the
	// "Downloads" folder wouldn't be selected in the treeview when navigating to it via its path.
	// From some investigation, it appears that's because the pidl returned by SHParseDisplayName()
	// would refer to an item in the root desktop folder, but not the "Downloads" item that normally
	// appears.
	// By passing the STR_PARSE_PREFER_FOLDER_BROWSING option, the pidl that's returned in that
	// situation will be for the filesystem folder specifically, which then means that folder will
	// be correctly selected in the treeview.
	// Note that the return value of this function is only DCHECK'd. It's not expected that the call
	// would fail, but if it does, bindCtx will be empty and the value passed through to
	// SHParseDisplayName() will be null, which is valid.
	wil::com_ptr_nothrow<IBindCtx> bindCtx;
	HRESULT hr = CreateBindCtxWithParam(STR_PARSE_PREFER_FOLDER_BROWSING, bindCtx);
	DCHECK(SUCCEEDED(hr));

	return SHParseDisplayName(itemPath.c_str(), bindCtx.get(), wil::out_param(pidlItem), 0,
		nullptr);
}

HRESULT MaybeGetLinkTarget(PCIDLIST_ABSOLUTE pidl, unique_pidl_absolute &targetPidl)
{
	return MaybeGetLinkTarget(nullptr, pidl, LinkTargetRetrievalType::DontResolve, targetPidl);
}

HRESULT MaybeResolveLinkTarget(HWND hwnd, PCIDLIST_ABSOLUTE pidl, unique_pidl_absolute &targetPidl)
{
	return MaybeGetLinkTarget(hwnd, pidl, LinkTargetRetrievalType::Resolve, targetPidl);
}

// If the specified item supports the IShellLink interface - that is, the item is a shortcut (i.e. a
// .lnk file), symlink (i.e. created by mklink) or virtual link object (e.g. an item in the quick
// access folder), this function will return the target pidl.
// Depending on the LinkTargetRetrievalType, the target will be resolved by the shell, which may
// result in a dialog being shown to the user (if the target doesn't currently exist).
HRESULT MaybeGetLinkTarget(HWND hwnd, PCIDLIST_ABSOLUTE pidl, LinkTargetRetrievalType retrievalType,
	unique_pidl_absolute &targetPidl)
{
	wil::com_ptr_nothrow<IShellItem> shellItem;
	RETURN_IF_FAILED(SHCreateItemFromIDList(pidl, IID_PPV_ARGS(&shellItem)));

	wil::com_ptr_nothrow<IShellLink> shellLink;
	RETURN_IF_FAILED(shellItem->BindToHandler(nullptr, BHID_SFUIObject, IID_PPV_ARGS(&shellLink)));

	if (retrievalType == LinkTargetRetrievalType::Resolve)
	{
		HRESULT hr = shellLink->Resolve(hwnd, SLR_UPDATE);

		if (FAILED(hr))
		{
			return hr;
		}

		// S_FALSE can be returned in situations like the following:
		//
		// When the target of a shortcut has been deleted and IShellLink::Resolve() is called, the
		// shell can show a dialog that contains three options:
		//
		// - One that allows the target to be restored from the recycle bin.
		// - One that allows the shortcut to be deleted.
		// - One that allows the operation to be canceled.
		//
		// In the latter two cases, S_FALSE will be returned and is effectively an error. That is,
		// if S_FALSE is returned, the target won't exist.
		if (hr == S_FALSE)
		{
			return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
		}
	}

	RETURN_IF_FAILED(shellLink->GetIDList(wil::out_param(targetPidl)));

	return S_OK;
}
