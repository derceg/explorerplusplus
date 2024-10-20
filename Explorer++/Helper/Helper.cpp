// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Helper.h"
#include "Macros.h"
#include "ShellHelper.h"
#include "TimeHelper.h"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <glog/logging.h>
#include <wil/com.h>
#include <wil/resource.h>
#include <WbemIdl.h>
#include <comutil.h>

// Required for CLSID_WbemLocator.
#pragma comment(lib, "wbemuuid.lib")

enum class VersionSubBlockType
{
	Root,
	Translation,
	StringTableValue
};

BOOL GetFileVersionValue(const TCHAR *szFullFileName, VersionSubBlockType subBlockType,
	WORD *pwLanguage, DWORD *pdwProductVersionLS, DWORD *pdwProductVersionMS,
	const TCHAR *szVersionInfo, TCHAR *szVersionBuffer, UINT cchMax);
BOOL GetStringTableValue(void *pBlock, LangAndCodePage *plcp, UINT nItems,
	const TCHAR *szVersionInfo, TCHAR *szVersionBuffer, UINT cchMax);

BOOL CreateFileTimeString(const FILETIME *utcFileTime, TCHAR *szBuffer, size_t cchMax,
	BOOL bFriendlyDate)
{
	SYSTEMTIME localSystemTime;
	BOOL ret = FileTimeToLocalSystemTime(utcFileTime, &localSystemTime);

	if (!ret)
	{
		return FALSE;
	}

	return CreateSystemTimeString(&localSystemTime, szBuffer, cchMax, bFriendlyDate);
}

BOOL CreateSystemTimeString(const SYSTEMTIME *localSystemTime, TCHAR *szBuffer, size_t cchMax,
	BOOL bFriendlyDate)
{
	if (bFriendlyDate)
	{
		BOOL ret = CreateFriendlySystemTimeString(localSystemTime, szBuffer, cchMax);

		if (ret)
		{
			return TRUE;
		}
	}

	TCHAR dateBuffer[512];
	int iReturn1 = GetDateFormat(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP, localSystemTime, nullptr,
		dateBuffer, SIZEOF_ARRAY(dateBuffer));

	TCHAR timeBuffer[512];
	int iReturn2 = GetTimeFormat(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP, localSystemTime, nullptr,
		timeBuffer, SIZEOF_ARRAY(timeBuffer));

	if ((iReturn1 != 0) && (iReturn2 != 0))
	{
		StringCchPrintf(szBuffer, cchMax, _T("%s %s"), dateBuffer, timeBuffer);
		return TRUE;
	}

	return FALSE;
}

BOOL CreateFriendlySystemTimeString(const SYSTEMTIME *localSystemTime, TCHAR *szBuffer,
	size_t cchMax)
{
	using namespace boost::gregorian;
	using namespace boost::posix_time;

	FILETIME localFileTime;
	BOOL ret = SystemTimeToFileTime(localSystemTime, &localFileTime);

	if (!ret)
	{
		return FALSE;
	}

	TCHAR dateComponent[512];
	bool dateComponentSet = false;

	auto inputPosixTime = from_ftime<ptime>(localFileTime);
	date inputDate = inputPosixTime.date();

	date today = day_clock::local_day();
	date yesterday = today - days(1);

	if (inputDate == today)
	{
		StringCchCopy(dateComponent, std::size(dateComponent), _T("Today"));
		dateComponentSet = true;
	}
	else if (inputDate == yesterday)
	{
		StringCchCopy(dateComponent, std::size(dateComponent), _T("Yesterday"));
		dateComponentSet = true;
	}

	if (!dateComponentSet)
	{
		return FALSE;
	}

	TCHAR timeComponent[512];
	int timeFormatted = GetTimeFormat(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP, localSystemTime,
		nullptr, timeComponent, SIZEOF_ARRAY(timeComponent));

	if (timeFormatted == 0)
	{
		return FALSE;
	}

	HRESULT hr = StringCchPrintf(szBuffer, cchMax, _T("%s, %s"), dateComponent, timeComponent);

	if (SUCCEEDED(hr))
	{
		return TRUE;
	}

	return FALSE;
}

HINSTANCE StartCommandPrompt(const std::wstring &directory, bool elevated)
{
	HINSTANCE hNewInstance = nullptr;

	TCHAR systemPath[MAX_PATH];
	BOOL bRes = SHGetSpecialFolderPath(nullptr, systemPath, CSIDL_SYSTEM, 0);

	if (bRes)
	{
		TCHAR commandPath[MAX_PATH];
		TCHAR *szRet = PathCombine(commandPath, systemPath, _T("cmd.exe"));

		if (szRet != nullptr)
		{
			TCHAR operation[32];
			std::wstring parameters;

			if (elevated)
			{
				StringCchCopy(operation, std::size(operation), _T("runas"));
				parameters = _T("/K cd /d ") + directory;
			}
			else
			{
				StringCchCopy(operation, std::size(operation), _T("open"));
			}

			hNewInstance = ShellExecute(nullptr, operation, commandPath, parameters.c_str(),
				directory.c_str(), SW_SHOWNORMAL);
		}
	}

	return hNewInstance;
}

BOOL GetFileSizeEx(const TCHAR *szFileName, PLARGE_INTEGER lpFileSize)
{
	BOOL bSuccess = FALSE;

	wil::unique_hfile file(CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr, OPEN_EXISTING, NULL, nullptr));

	if (file)
	{
		bSuccess = GetFileSizeEx(file.get(), lpFileSize);
	}

	return bSuccess;
}

BOOL CompareFileTypes(const TCHAR *pszFile1, const TCHAR *pszFile2)
{
	SHFILEINFO shfi1;
	SHFILEINFO shfi2;

	DWORD_PTR result1 = SHGetFileInfo(pszFile1, NULL, &shfi1, sizeof(shfi1), SHGFI_TYPENAME);
	DWORD_PTR result2 = SHGetFileInfo(pszFile2, NULL, &shfi2, sizeof(shfi2), SHGFI_TYPENAME);

	if (result1 != 0 && result2 != 0 && StrCmp(shfi1.szTypeName, shfi2.szTypeName) == 0)
	{
		return TRUE;
	}

	return FALSE;
}

std::wstring BuildFileAttributesString(DWORD fileAttributes)
{
	auto appendAttribute = [](std::wstring &text, bool isAttributeSet, WCHAR attributeCharacter)
	{
		if (isAttributeSet)
		{
			text += attributeCharacter;
		}
		else
		{
			text += '-';
		}
	};

	std::wstring attributesString;
	appendAttribute(attributesString, WI_IsFlagSet(fileAttributes, FILE_ATTRIBUTE_ARCHIVE), 'A');
	appendAttribute(attributesString, WI_IsFlagSet(fileAttributes, FILE_ATTRIBUTE_HIDDEN), 'H');
	appendAttribute(attributesString, WI_IsFlagSet(fileAttributes, FILE_ATTRIBUTE_READONLY), 'R');
	appendAttribute(attributesString, WI_IsFlagSet(fileAttributes, FILE_ATTRIBUTE_SYSTEM), 'S');
	appendAttribute(attributesString, WI_IsFlagSet(fileAttributes, FILE_ATTRIBUTE_DIRECTORY), 'D');
	appendAttribute(attributesString, WI_IsFlagSet(fileAttributes, FILE_ATTRIBUTE_COMPRESSED), 'C');
	appendAttribute(attributesString, WI_IsFlagSet(fileAttributes, FILE_ATTRIBUTE_SPARSE_FILE),
		'P');
	appendAttribute(attributesString, WI_IsFlagSet(fileAttributes, FILE_ATTRIBUTE_ENCRYPTED), 'E');
	appendAttribute(attributesString,
		WI_IsFlagSet(fileAttributes, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED), 'I');

	return attributesString;
}

BOOL GetFileOwner(const TCHAR *szFile, TCHAR *szOwner, size_t cchMax)
{
	BOOL success = FALSE;

	wil::unique_hfile file(CreateFile(szFile, READ_CONTROL, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS, nullptr));

	if (file)
	{
		PSID pSidOwner = nullptr;
		PSECURITY_DESCRIPTOR pSD = nullptr;
		DWORD dwRet = GetSecurityInfo(file.get(), SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION,
			&pSidOwner, nullptr, nullptr, nullptr, &pSD);

		if (dwRet == ERROR_SUCCESS)
		{
			success = FormatUserName(pSidOwner, szOwner, cchMax);
			LocalFree(pSD);
		}
	}

	return success;
}

BOOL FormatUserName(PSID sid, TCHAR *userName, size_t cchMax)
{
	BOOL success = FALSE;

	TCHAR accountName[512];
	DWORD accountNameLength = SIZEOF_ARRAY(accountName);
	TCHAR domainName[512];
	DWORD domainNameLength = SIZEOF_ARRAY(domainName);
	SID_NAME_USE eUse;
	BOOL bRet = LookupAccountSid(nullptr, sid, accountName, &accountNameLength, domainName,
		&domainNameLength, &eUse);

	if (bRet)
	{
		StringCchPrintf(userName, cchMax, _T("%s\\%s"), domainName, accountName);
		success = TRUE;
	}
	else
	{
		LPTSTR stringSid;
		bRet = ConvertSidToStringSid(sid, &stringSid);

		if (bRet)
		{
			StringCchCopy(userName, cchMax, stringSid);

			LocalFree(stringSid);
			success = TRUE;
		}
	}

	return success;
}

BOOL CheckGroupMembership(GroupType groupType)
{
	SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
	PSID psid;
	DWORD dwGroup = 0;
	BOOL bMember = FALSE;
	BOOL bRet;

	switch (groupType)
	{
	case GroupType::Administrators:
		dwGroup = DOMAIN_ALIAS_RID_ADMINS;
		break;

	case GroupType::PowerUsers:
		dwGroup = DOMAIN_ALIAS_RID_POWER_USERS;
		break;

	case GroupType::Users:
		dwGroup = DOMAIN_ALIAS_RID_USERS;
		break;

	case GroupType::UsersRestricted:
		dwGroup = DOMAIN_ALIAS_RID_GUESTS;
		break;
	}

	bRet = AllocateAndInitializeSid(&sia, 2, SECURITY_BUILTIN_DOMAIN_RID, dwGroup, 0, 0, 0, 0, 0, 0,
		&psid);

	if (bRet)
	{
		CheckTokenMembership(nullptr, psid, &bMember);

		FreeSid(psid);
	}

	return bMember;
}

DWORD GetNumFileHardLinks(const TCHAR *lpszFileName)
{
	DWORD nLinks = 0;

	wil::unique_hfile file(CreateFile(lpszFileName, FILE_READ_ATTRIBUTES, FILE_SHARE_READ, nullptr,
		OPEN_EXISTING, NULL, nullptr));

	if (file)
	{
		BY_HANDLE_FILE_INFORMATION fileInfo;
		BOOL bRet = GetFileInformationByHandle(file.get(), &fileInfo);

		if (bRet)
		{
			nLinks = fileInfo.nNumberOfLinks;
		}
	}

	return nLinks;
}

BOOL ReadImageProperty(const TCHAR *lpszImage, PROPID propId, TCHAR *szProperty, int cchMax)
{
	BOOL bSuccess = FALSE;

	/* This object needs to be
	deleted before GdiplusShutdown
	is called. If it were stack
	allocated, it would go out of
	scope _after_ the call to
	GdiplusShutdown. By allocating
	it on the heap, the lifetime
	can be directly controlled. */
	auto *image = new Gdiplus::Image(lpszImage, FALSE);

	if (image->GetLastStatus() == Gdiplus::Ok)
	{
		if (propId == PropertyTagImageWidth)
		{
			bSuccess = TRUE;
			StringCchPrintf(szProperty, cchMax, _T("%u pixels"), image->GetWidth());
		}
		else if (propId == PropertyTagImageHeight)
		{
			bSuccess = TRUE;
			StringCchPrintf(szProperty, cchMax, _T("%u pixels"), image->GetHeight());
		}
		else
		{
			UINT size = image->GetPropertyItemSize(propId);

			if (size != 0)
			{
				auto *propertyItem = reinterpret_cast<Gdiplus::PropertyItem *>(malloc(size));

				if (propertyItem != nullptr)
				{
					Gdiplus::Status status = image->GetPropertyItem(propId, size, propertyItem);

					if (status == Gdiplus::Ok)
					{
						if (propertyItem->type == PropertyTagTypeASCII)
						{
							int iRes = MultiByteToWideChar(CP_ACP, 0,
								reinterpret_cast<LPCSTR>(propertyItem->value), -1, szProperty,
								cchMax);

							if (iRes != 0)
							{
								bSuccess = TRUE;
							}
						}
					}

					free(propertyItem);
				}
			}
		}
	}

	delete image;

	return bSuccess;
}

BOOL GetFileNameFromUser(HWND hwnd, TCHAR *fullFileName, UINT cchMax, const TCHAR *initialDirectory)
{
	/* As per the documentation for
	the OPENFILENAME structure, the
	length of the filename buffer
	should be at least 256. */
	assert(cchMax >= 256);

	const TCHAR *filter = _T("Text Document (*.txt)\0*.txt\0All Files\0*.*\0\0");
	OPENFILENAME ofn;
	BOOL bRet;

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = filter;
	ofn.lpstrCustomFilter = nullptr;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = fullFileName;
	ofn.nMaxFile = cchMax;
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = initialDirectory;
	ofn.lpstrTitle = nullptr;
	ofn.Flags = OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_EXPLORER;
	ofn.lpstrDefExt = _T("txt");
	ofn.lCustData = NULL;
	ofn.lpfnHook = nullptr;
	ofn.pvReserved = nullptr;
	ofn.dwReserved = NULL;
	ofn.FlagsEx = NULL;

	bRet = GetSaveFileName(&ofn);

	return bRet;
}

BOOL IsImage(const TCHAR *szFileName)
{
	static const TCHAR *IMAGE_EXTS[] = { _T("bmp"), _T("ico"), _T("gif"), _T("jpg"), _T("exf"),
		_T("png"), _T("tif"), _T("wmf"), _T("emf"), _T("tiff") };
	TCHAR *ext;
	int i = 0;

	ext = PathFindExtension(szFileName);

	if (ext == nullptr || (ext + 1) == nullptr)
	{
		return FALSE;
	}

	ext++;

	for (i = 0; i < SIZEOF_ARRAY(IMAGE_EXTS); i++)
	{
		if (lstrcmpi(ext, IMAGE_EXTS[i]) == 0)
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOL GetFileProductVersion(const TCHAR *szFullFileName, DWORD *pdwProductVersionLS,
	DWORD *pdwProductVersionMS)
{
	return GetFileVersionValue(szFullFileName, VersionSubBlockType::Root, nullptr,
		pdwProductVersionLS, pdwProductVersionMS, nullptr, nullptr, 0);
}

BOOL GetFileLanguage(const TCHAR *szFullFileName, WORD *pwLanguage)
{
	return GetFileVersionValue(szFullFileName, VersionSubBlockType::Translation, pwLanguage,
		nullptr, nullptr, nullptr, nullptr, 0);
}

BOOL GetVersionInfoString(const TCHAR *szFullFileName, const TCHAR *szVersionInfo,
	TCHAR *szVersionBuffer, UINT cchMax)
{
	return GetFileVersionValue(szFullFileName, VersionSubBlockType::StringTableValue, nullptr,
		nullptr, nullptr, szVersionInfo, szVersionBuffer, cchMax);
}

BOOL GetFileVersionValue(const TCHAR *szFullFileName, VersionSubBlockType subBlockType,
	WORD *pwLanguage, DWORD *pdwProductVersionLS, DWORD *pdwProductVersionMS,
	const TCHAR *szVersionInfo, TCHAR *szVersionBuffer, UINT cchMax)
{
	BOOL bSuccess = FALSE;
	DWORD dwLen = GetFileVersionInfoSize(szFullFileName, nullptr);

	if (dwLen > 0)
	{
		void *pBlock = malloc(dwLen);

		if (pBlock != nullptr)
		{
			BOOL bRet = GetFileVersionInfo(szFullFileName, NULL, dwLen, pBlock);

			if (bRet)
			{
				TCHAR szSubBlock[64];
				LPVOID *pBuffer = nullptr;
				UINT uStructureSize = 0;

				LangAndCodePage *plcp = nullptr;
				VS_FIXEDFILEINFO *pvsffi = nullptr;

				if (subBlockType == VersionSubBlockType::Root)
				{
					StringCchCopy(szSubBlock, std::size(szSubBlock), _T("\\"));
					pBuffer = reinterpret_cast<LPVOID *>(&pvsffi);
					uStructureSize = sizeof(VS_FIXEDFILEINFO);
				}
				else if (subBlockType == VersionSubBlockType::Translation
					|| subBlockType == VersionSubBlockType::StringTableValue)
				{
					StringCchCopy(szSubBlock, std::size(szSubBlock),
						_T("\\VarFileInfo\\Translation"));
					pBuffer = reinterpret_cast<LPVOID *>(&plcp);
					uStructureSize = sizeof(LangAndCodePage);
				}

				UINT uLen;
				bRet = VerQueryValue(pBlock, szSubBlock, pBuffer, &uLen);

				if (bRet && (uLen >= uStructureSize))
				{
					bSuccess = TRUE;

					if (subBlockType == VersionSubBlockType::Root)
					{
						*pdwProductVersionLS = pvsffi->dwProductVersionLS;
						*pdwProductVersionMS = pvsffi->dwProductVersionMS;
					}
					else if (subBlockType == VersionSubBlockType::Translation)
					{
						*pwLanguage = plcp[0].wLanguage;
					}
					else if (subBlockType == VersionSubBlockType::StringTableValue)
					{
						bSuccess = GetStringTableValue(pBlock, plcp, uLen / sizeof(LangAndCodePage),
							szVersionInfo, szVersionBuffer, cchMax);
					}
				}
			}

			free(pBlock);
		}
	}

	return bSuccess;
}

BOOL GetStringTableValue(void *pBlock, LangAndCodePage *plcp, UINT nItems,
	const TCHAR *szVersionInfo, TCHAR *szVersionBuffer, UINT cchMax)
{
	BOOL bSuccess = FALSE;
	LANGID userLangId = GetUserDefaultLangID();

	for (UINT i = 0; i < nItems; i++)
	{
		/* If the bottom eight bits of the language id's match, use this
		version information (since this means that the version information
		and the users default language are the same). Also use this version
		information if the language is not specified (i.e. wLanguage is 0). */
		if ((plcp[i].wLanguage & 0xFF) == (userLangId & 0xFF) || plcp[i].wLanguage == 0)
		{
			TCHAR szSubBlock[64];
			StringCchPrintf(szSubBlock, SIZEOF_ARRAY(szSubBlock),
				_T("\\StringFileInfo\\%04X%04X\\%s"), plcp[i].wLanguage, plcp[i].wCodePage,
				szVersionInfo);

			TCHAR *szBuffer;
			UINT uLen;
			BOOL bRet =
				VerQueryValue(pBlock, szSubBlock, reinterpret_cast<LPVOID *>(&szBuffer), &uLen);

			if (bRet && (uLen > 0))
			{
				StringCchCopy(szVersionBuffer, cchMax, szBuffer);
				bSuccess = TRUE;
				break;
			}
		}
	}

	return bSuccess;
}

HRESULT GetCPUBrandString(std::wstring &cpuBrand)
{
	// The code below is modeled very closely on the example WMI code provided in
	// https://learn.microsoft.com/en-us/windows/win32/wmisdk/example--getting-wmi-data-from-the-local-computer.
	wil::com_ptr_nothrow<IWbemLocator> locator;
	RETURN_IF_FAILED(
		CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&locator)));

	wil::com_ptr_nothrow<IWbemServices> service;
	RETURN_IF_FAILED(locator->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, nullptr, 0,
		nullptr, nullptr, &service));

	RETURN_IF_FAILED(CoSetProxyBlanket(service.get(), RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
		RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE));

	wil::com_ptr_nothrow<IEnumWbemClassObject> enumerator;
	RETURN_IF_FAILED(service->ExecQuery(bstr_t("WQL"), bstr_t("SELECT Name FROM Win32_Processor"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &enumerator));

	wil::com_ptr_nothrow<IWbemClassObject> object;
	ULONG numReturned;
	enumerator->Next(WBEM_INFINITE, 1, &object, &numReturned);

	if (numReturned == 0)
	{
		return E_FAIL;
	}

	wil::unique_variant property;
	RETURN_IF_FAILED(object->Get(L"Name", 0, &property, nullptr, nullptr));

	// As per the documentation for the Win32_Processor WMI class, the name field is a string, which
	// means it's safe to assume that here (rather than having to perform a string conversion on a
	// generic VARIANT).
	cpuBrand = ConvertBstrToString(V_BSTR(&property));

	return S_OK;
}

HRESULT GetMediaMetadata(const TCHAR *szFileName, const TCHAR *szAttribute, BYTE **pszOutput)
{
	typedef HRESULT(WINAPI * WMCREATEEDITOR_PROC)(IWMMetadataEditor **);
	WMCREATEEDITOR_PROC pWMCreateEditor = nullptr;
	HMODULE hWMVCore;
	IWMMetadataEditor *pEditor = nullptr;
	IWMHeaderInfo *pWMHeaderInfo = nullptr;
	HRESULT hr = E_FAIL;

	hWMVCore = LoadLibrary(_T("wmvcore.dll"));

	if (hWMVCore != nullptr)
	{
		pWMCreateEditor = (WMCREATEEDITOR_PROC) GetProcAddress(hWMVCore, "WMCreateEditor");

		if (pWMCreateEditor != nullptr)
		{
			hr = pWMCreateEditor(&pEditor);

			if (SUCCEEDED(hr))
			{
				hr = pEditor->Open(szFileName);

				if (SUCCEEDED(hr))
				{
					hr = pEditor->QueryInterface(IID_PPV_ARGS(&pWMHeaderInfo));

					if (SUCCEEDED(hr))
					{
						WORD wStreamNum;
						WMT_ATTR_DATATYPE type;
						WORD cbLength;

						/* Any stream. Should be zero for MP3 files. */
						wStreamNum = 0;

						hr = pWMHeaderInfo->GetAttributeByName(&wStreamNum, szAttribute, &type,
							nullptr, &cbLength);

						if (SUCCEEDED(hr))
						{
							*pszOutput = (BYTE *) malloc(cbLength);

							if (*pszOutput != nullptr)
							{
								hr = pWMHeaderInfo->GetAttributeByName(&wStreamNum, szAttribute,
									&type, *pszOutput, &cbLength);
							}
						}

						pWMHeaderInfo->Release();
					}
				}

				pEditor->Release();
			}
		}

		FreeLibrary(hWMVCore);
	}

	return hr;
}

void SetFORMATETC(FORMATETC *pftc, CLIPFORMAT cfFormat, DVTARGETDEVICE *ptd, DWORD dwAspect,
	LONG lindex, DWORD tymed)
{
	pftc->cfFormat = cfFormat;
	pftc->tymed = tymed;
	pftc->lindex = lindex;
	pftc->dwAspect = dwAspect;
	pftc->ptd = ptd;
}

bool IsKeyDown(int nVirtKey)
{
	SHORT status = (GetKeyState(nVirtKey) & 0x8000);

	return (status != 0);
}

// Generates a simulated keypress by sending WM_KEYDOWN/WM_KEYUP messages to the specified window.
// This isn't a general solution to sending input and should only be used if needed to invoke
// specific behavior in a child window.
void SendSimulatedKeyPress(HWND hwnd, UINT key)
{
	UINT scanCode = MapVirtualKey(key, MAPVK_VK_TO_VSC);

	if (scanCode == 0)
	{
		DCHECK(false);
		return;
	}

	int repeatCount = 1;
	LPARAM additionalInfo = repeatCount | (scanCode << 16);
	UINT flags = 0;

	if (IsExtendedKey(key))
	{
		flags |= KF_EXTENDED;
	}

	additionalInfo |= (flags << 16);

	SendMessage(hwnd, WM_KEYDOWN, key, additionalInfo);

	// Both of these flags are always set for WM_KEYUP messages.
	flags |= KF_REPEAT;
	flags |= KF_UP;
	additionalInfo |= (flags << 16);

	SendMessage(hwnd, WM_KEYUP, key, additionalInfo);
}

bool IsExtendedKey(UINT key)
{
	// clang-format off
	if (key == VK_LEFT
		|| key == VK_UP
		|| key == VK_RIGHT
		|| key == VK_DOWN
		|| key == VK_RCONTROL
		|| key == VK_RMENU
		|| key == VK_LWIN
		|| key == VK_RWIN
		|| key == VK_APPS
		|| key == VK_PRIOR
		|| key == VK_NEXT
		|| key == VK_END
		|| key == VK_HOME
		|| key == VK_INSERT
		|| key == VK_DELETE
		|| key == VK_DIVIDE
		|| key == VK_NUMLOCK
		|| key == VK_ADD
		|| key == VK_SUBTRACT)
	// clang-format on
	{
		return true;
	}

	return false;
}

std::wstring CreateGUID()
{
	GUID guid;
	CoCreateGuid(&guid);

	TCHAR guidString[128];
	StringFromGUID2(guid, guidString, SIZEOF_ARRAY(guidString));

	std::wstring finalValue = guidString;
	finalValue = finalValue.substr(1, finalValue.length() - 2);

	return finalValue;
}

std::optional<std::wstring> GetLastErrorMessage(DWORD error)
{
	wil::unique_hlocal_string systemErrorMessage;
	DWORD size = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS
			| FORMAT_MESSAGE_ALLOCATE_BUFFER,
		nullptr, error, 0, reinterpret_cast<LPWSTR>(&systemErrorMessage), 32 * 1024, nullptr);

	if (size > 0)
	{
		return systemErrorMessage.get();
	}

	return std::nullopt;
}

bool IsWindowsPE()
{
	// As per
	// https://groups.google.com/g/microsoft.public.win32.programmer.kernel/c/jam056kRtBA/m/Nsoadoca4IUJ,
	// the MiniNT key will only be present on Windows PE.
	wil::unique_hkey key;
	LSTATUS result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"System\\ControlSet001\\Control\\MiniNT", 0,
		KEY_READ, &key);
	return result == ERROR_SUCCESS;
}

bool IsProcessRTL()
{
	DWORD layout = 0;
	BOOL res = GetProcessDefaultLayout(&layout);

	if (!res)
	{
		LOG_SYSRESULT(GetLastError());
		DCHECK(false);
		return false;
	}

	return (layout == LAYOUT_RTL);
}
