// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FileOperations.h"
#include "DragDropHelper.h"
#include "DriveInfo.h"
#include "Helper.h"
#include "ShellHelper.h"
#include "StringHelper.h"
#include <filesystem>
#include <list>
#include <sstream>

BOOL GetFileClusterSize(const std::wstring &strFilename, PLARGE_INTEGER lpRealFileSize);

HRESULT FileOperations::RenameFile(IShellItem *item, const std::wstring &newName)
{
	wil::com_ptr_nothrow<IFileOperation> fo;
	HRESULT hr = CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&fo));

	if (FAILED(hr))
	{
		return hr;
	}

	hr = fo->SetOperationFlags(FOF_ALLOWUNDO | FOF_SILENT);

	if (FAILED(hr))
	{
		return hr;
	}

	hr = fo->RenameItem(item, newName.c_str(), nullptr);

	if (FAILED(hr))
	{
		return hr;
	}

	hr = fo->PerformOperations();

	return hr;
}

HRESULT FileOperations::DeleteFiles(HWND hwnd, std::vector<PCIDLIST_ABSOLUTE> &pidls,
	bool permanent, bool silent)
{
	wil::com_ptr_nothrow<IFileOperation> fo{};
	auto hr = CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&fo));
	if (FAILED(hr) || !fo)
	{
		return hr;
	}

	wil::com_ptr_nothrow<IFileOperationProgressSink> sink{};
	hr = CreateFileOperationProgressSink(pidls, &sink);
	if (FAILED(hr) || !sink)
	{
		return hr;
	}

	DWORD cookie{};
	hr = fo->Advise(sink.get(), &cookie);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = fo->SetOwnerWindow(hwnd);

	if (FAILED(hr))
	{
		return hr;
	}

	DWORD flags = 0;

	if (!permanent)
	{
		flags |= FOF_ALLOWUNDO;
	}

	if (silent)
	{
		flags |= FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI;
	}
	else
	{
		flags |= FOF_WANTNUKEWARNING;
	}

	if (flags != 0)
	{
		hr = fo->SetOperationFlags(flags);

		if (FAILED(hr))
		{
			return hr;
		}
	}

	wil::com_ptr_nothrow<IShellItemArray> shellItemArray{};
	hr = SHCreateShellItemArrayFromIDLists(static_cast<UINT>(pidls.size()), &pidls[0], &shellItemArray);

	if (FAILED(hr))
	{
		return hr;
	}

	wil::com_ptr_nothrow<IUnknown> unknown;
	hr = shellItemArray->QueryInterface(IID_PPV_ARGS(&unknown));

	if (FAILED(hr))
	{
		return hr;
	}

	hr = fo->DeleteItems(unknown.get());

	if (FAILED(hr))
	{
		return hr;
	}

	hr = fo->PerformOperations();
	fo->Unadvise(cookie);

	return hr;
}

HRESULT FileOperations::CopyFilesToFolder(HWND hOwner, const std::wstring &strTitle,
	std::vector<PCIDLIST_ABSOLUTE> &pidls, bool move)
{
	unique_pidl_absolute pidl;
	BOOL bRes = CreateBrowseDialog(hOwner, strTitle, wil::out_param(pidl));

	if (!bRes)
	{
		return E_FAIL;
	}

	wil::com_ptr_nothrow<IShellItem> destinationFolder;
	HRESULT hr = SHCreateItemFromIDList(pidl.get(), IID_PPV_ARGS(&destinationFolder));

	if (FAILED(hr))
	{
		return E_FAIL;
	}

	hr = CopyFiles(hOwner, destinationFolder.get(), pidls, move);

	return hr;
}

HRESULT FileOperations::CopyFiles(HWND hwnd, IShellItem *destinationFolder,
	std::vector<PCIDLIST_ABSOLUTE> &pidls, bool move)
{
	wil::com_ptr_nothrow<IFileOperation> fo;
	HRESULT hr = CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&fo));

	if (FAILED(hr))
	{
		return hr;
	}

	hr = fo->SetOwnerWindow(hwnd);

	if (FAILED(hr))
	{
		return hr;
	}

	hr = fo->SetOperationFlags(FOF_ALLOWUNDO);

	if (FAILED(hr))
	{
		return hr;
	}

	wil::com_ptr_nothrow<IShellItemArray> shellItemArray;
	hr = SHCreateShellItemArrayFromIDLists(static_cast<UINT>(pidls.size()), &pidls[0],
		&shellItemArray);

	if (FAILED(hr))
	{
		return hr;
	}

	wil::com_ptr_nothrow<IUnknown> unknown;
	hr = shellItemArray->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&unknown));

	if (FAILED(hr))
	{
		return hr;
	}

	if (move)
	{
		hr = fo->MoveItems(unknown.get(), destinationFolder);
	}
	else
	{
		hr = fo->CopyItems(unknown.get(), destinationFolder);
	}

	if (FAILED(hr))
	{
		return hr;
	}

	hr = fo->PerformOperations();

	return hr;
}

TCHAR *FileOperations::BuildFilenameList(const std::list<std::wstring> &FilenameList)
{
	TCHAR *pszFilenames = nullptr;
	int iTotalSize = 0;

	for (const auto &filename : FilenameList)
	{
		pszFilenames = reinterpret_cast<TCHAR *>(
			realloc(pszFilenames, (iTotalSize + filename.size() + 1) * sizeof(TCHAR)));
		memcpy(pszFilenames + iTotalSize, filename.c_str(), (filename.size() + 1) * sizeof(TCHAR));
		iTotalSize += static_cast<int>(filename.size() + 1);
	}

	/* The list of strings must end with a second
	terminating NULL character, so add it now. */
	pszFilenames =
		reinterpret_cast<TCHAR *>(realloc(pszFilenames, (iTotalSize + 1) * sizeof(TCHAR)));
	pszFilenames[iTotalSize] = '\0';

	/* Note that it is up to the caller to free this. */
	return pszFilenames;
}

// Creates a new folder. Note that IFileOperation will take care of
// renaming the folder if one with that name already exists.
HRESULT FileOperations::CreateNewFolder(IShellItem *destinationFolder,
	const std::wstring &newFolderName, IFileOperationProgressSink *progressSink)
{
	wil::com_ptr_nothrow<IFileOperation> fo;
	HRESULT hr = CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&fo));

	if (FAILED(hr))
	{
		return hr;
	}

	hr = fo->SetOperationFlags(FOF_ALLOWUNDO | FOF_SILENT);

	if (FAILED(hr))
	{
		return hr;
	}

	hr = fo->NewItem(destinationFolder, FILE_ATTRIBUTE_DIRECTORY, newFolderName.c_str(), nullptr,
		progressSink);

	if (FAILED(hr))
	{
		return hr;
	}

	hr = fo->PerformOperations();

	return hr;
}

BOOL FileOperations::SaveDirectoryListing(const std::wstring &strDirectory,
	const std::wstring &strFilename)
{
	std::wstring strContents = _T("Directory\r\n---------\r\n") + strDirectory + _T("\r\n\r\n");

	SYSTEMTIME st;
	FILETIME ft;
	FILETIME lft;
	GetLocalTime(&st);
	SystemTimeToFileTime(&st, &ft);
	LocalFileTimeToFileTime(&ft, &lft);

	TCHAR szTime[128];
	CreateFileTimeString(&lft, szTime, std::size(szTime), FALSE);
	strContents += _T("Date\r\n----\r\n") + std::wstring(szTime) + _T("\r\n\r\n");

	std::wstring strSearch = strDirectory + _T("\\*");

	WIN32_FIND_DATA wfd;
	HANDLE hFirstFile = FindFirstFile(strSearch.c_str(), &wfd);

	std::list<std::wstring> folderList;
	std::list<std::wstring> fileList;
	ULARGE_INTEGER ulTotalSize;

	ulTotalSize.QuadPart = 0;

	if (hFirstFile != INVALID_HANDLE_VALUE)
	{
		ULARGE_INTEGER ulFileSize;

		if (lstrcmpi(wfd.cFileName, _T(".")) != 0 && lstrcmpi(wfd.cFileName, _T("..")) != 0)
		{
			if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
			{
				folderList.emplace_back(wfd.cFileName);
			}
			else
			{
				fileList.emplace_back(wfd.cFileName);

				ulFileSize.LowPart = wfd.nFileSizeLow;
				ulFileSize.HighPart = wfd.nFileSizeHigh;

				ulTotalSize.QuadPart += ulFileSize.QuadPart;
			}
		}

		while (FindNextFile(hFirstFile, &wfd) != 0)
		{
			if (lstrcmpi(wfd.cFileName, _T(".")) != 0 && lstrcmpi(wfd.cFileName, _T("..")) != 0)
			{
				if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
				{
					folderList.emplace_back(wfd.cFileName);
				}
				else
				{
					fileList.emplace_back(wfd.cFileName);

					ulFileSize.LowPart = wfd.nFileSizeLow;
					ulFileSize.HighPart = wfd.nFileSizeHigh;

					ulTotalSize.QuadPart += ulFileSize.QuadPart;
				}
			}
		}

		FindClose(hFirstFile);
	}

	std::wstringstream ss;
	ss.imbue(std::locale(""));
	ss.precision(0);

	strContents += _T("Statistics\r\n----------\r\n");

	ss << std::fixed << folderList.size();
	strContents += _T("Number of folders: ") + ss.str() + std::wstring(_T("\r\n"));

	ss.str(_T(""));
	ss << std::fixed << fileList.size();
	strContents += _T("Number of files: ") + ss.str() + std::wstring(_T("\r\n"));

	auto totalSizeText = FormatSizeString(ulTotalSize.QuadPart);
	strContents += _T("Total size (not including subfolders): ") + totalSizeText + _T("\r\n");

	strContents += _T("\r\nFolders\r\n-------\r\n");

	for (const auto &folder : folderList)
	{
		strContents += folder + _T("\r\n");
	}

	strContents += _T("\r\nFiles\r\n-----\r\n");

	for (const auto &file : fileList)
	{
		strContents += file + _T("\r\n");
	}

	/* Remove the trailing newline. */
	strContents = strContents.substr(0, strContents.size() - 2);

	HANDLE hFile = CreateFile(strFilename.c_str(), FILE_WRITE_DATA, 0, nullptr, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, nullptr);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD nBytesWritten;

		/* Write out the BOM for UTF-16 LE data.
		See http://en.wikipedia.org/wiki/Byte-order_mark */
		WriteFile(hFile, reinterpret_cast<LPCVOID>("\xFF\xFE"), 2, &nBytesWritten, nullptr);

		WriteFile(hFile, reinterpret_cast<LPCVOID>(strContents.c_str()),
			static_cast<DWORD>(strContents.size() * sizeof(WCHAR)), &nBytesWritten, nullptr);

		if (nBytesWritten == strContents.size())
		{
			return TRUE;
		}

		CloseHandle(hFile);
	}

	return FALSE;
}

HRESULT CopyFiles(const std::vector<PidlAbsolute> &items, IDataObject **dataObjectOut)
{
	return CopyFilesToClipboard(items, false, dataObjectOut);
}

HRESULT CutFiles(const std::vector<PidlAbsolute> &items, IDataObject **dataObjectOut)
{
	return CopyFilesToClipboard(items, true, dataObjectOut);
}

HRESULT CopyFilesToClipboard(const std::vector<PidlAbsolute> &items, bool move,
	IDataObject **dataObjectOut)
{
	wil::com_ptr_nothrow<IDataObject> dataObject;
	RETURN_IF_FAILED(CreateDataObjectForShellTransfer(items, &dataObject));

	DWORD effect = move ? DROPEFFECT_MOVE : DROPEFFECT_COPY;

	if (!move)
	{
		WI_SetFlag(effect, DROPEFFECT_LINK);
	}

	RETURN_IF_FAILED(SetPreferredDropEffect(dataObject.get(), effect));

	RETURN_IF_FAILED(OleSetClipboard(dataObject.get()));

	*dataObjectOut = dataObject.detach();

	return S_OK;
}

HRESULT FileOperations::CreateLinkToFile(const std::wstring &strTargetFilename,
	const std::wstring &strLinkFilename, const std::wstring &strLinkDescription)
{
	IShellLink *pShellLink = nullptr;
	HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pShellLink));

	if (SUCCEEDED(hr))
	{
		pShellLink->SetPath(strTargetFilename.c_str());
		pShellLink->SetDescription(strLinkDescription.c_str());

		IPersistFile *pPersistFile = nullptr;
		hr = pShellLink->QueryInterface(IID_PPV_ARGS(&pPersistFile));

		if (SUCCEEDED(hr))
		{
			pPersistFile->Save(strLinkFilename.c_str(), TRUE);
			pPersistFile->Release();
		}

		pShellLink->Release();
	}

	return hr;
}

HRESULT CreateFileOperationProgressSink(std::vector<PCIDLIST_ABSOLUTE> &pidls,
	IFileOperationProgressSink **ppSink)
{
	CFileOperationProgressSink* pfo = new (std::nothrow)CFileOperationProgressSink(pidls);
	if (!pfo)
		return E_OUTOFMEMORY;
	const auto hr = pfo->QueryInterface(IID_IFileOperationProgressSink, reinterpret_cast<LPVOID*>(ppSink));
	pfo->Release();
	return hr;
}

HRESULT FileOperations::ResolveLink(HWND hwnd, DWORD fFlags, const TCHAR *szLinkFilename,
	TCHAR *szResolvedPath, int nBufferSize)
{
	SHFILEINFO shfi;
	DWORD_PTR dwRet = SHGetFileInfo(szLinkFilename, NULL, &shfi, sizeof(shfi), SHGFI_ATTRIBUTES);

	if (dwRet == 0 || !(shfi.dwAttributes & SFGAO_LINK))
	{
		return E_FAIL;
	}

	IShellLink *pShellLink = nullptr;
	HRESULT hr =
		CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pShellLink));

	if (hr == S_OK)
	{
		IPersistFile *pPersistFile = nullptr;
		hr = pShellLink->QueryInterface(IID_PPV_ARGS(&pPersistFile));

		if (hr == S_OK)
		{
			hr = pPersistFile->Load(szLinkFilename, STGM_READ);

			if (hr == S_OK)
			{
				pShellLink->Resolve(hwnd, fFlags);

				TCHAR szResolvedPathInternal[MAX_PATH];
				pShellLink->GetPath(szResolvedPathInternal, std::size(szResolvedPathInternal),
					nullptr, SLGP_UNCPRIORITY);

				StringCchCopy(szResolvedPath, nBufferSize, szResolvedPathInternal);
			}

			pPersistFile->Release();
		}

		pShellLink->Release();
	}

	return hr;
}

BOOL FileOperations::CreateBrowseDialog(HWND hOwner, const std::wstring &strTitle,
	PIDLIST_ABSOLUTE *ppidl)
{
	TCHAR szDisplayName[MAX_PATH];

	BROWSEINFO bi;
	bi.hwndOwner = hOwner;
	bi.pidlRoot = nullptr;
	bi.pszDisplayName = szDisplayName;
	bi.lpszTitle = strTitle.c_str();
	bi.ulFlags = BIF_NEWDIALOGSTYLE;
	bi.lpfn = nullptr;
	*ppidl = SHBrowseForFolder(&bi);

	BOOL bSuccessful = (*ppidl != nullptr);

	return bSuccessful;
}

BOOL GetFileClusterSize(const std::wstring &strFilename, PLARGE_INTEGER lpRealFileSize)
{
	DWORD dwClusterSize;

	LARGE_INTEGER lFileSize;
	BOOL bRet = GetFileSizeEx(strFilename.c_str(), &lFileSize);

	if (!bRet)
	{
		return FALSE;
	}

	TCHAR szRoot[MAX_PATH];
	HRESULT hr = StringCchCopy(szRoot, std::size(szRoot), strFilename.c_str());

	if (FAILED(hr))
	{
		return FALSE;
	}

	bRet = PathStripToRoot(szRoot);

	if (!bRet)
	{
		return FALSE;
	}

	bRet = GetClusterSize(szRoot, &dwClusterSize);

	if (!bRet)
	{
		return FALSE;
	}

	if ((lFileSize.QuadPart % dwClusterSize) != 0)
	{
		/* The real size is the logical file size rounded up to the end of the
		nearest cluster. */
		lFileSize.QuadPart += dwClusterSize - (lFileSize.QuadPart % dwClusterSize);
	}

	*lpRealFileSize = lFileSize;

	return TRUE;
}

void FileOperations::DeleteFileSecurely(const std::wstring &strFilename,
	OverwriteMethod overwriteMethod)
{
	HANDLE hFile;
	WIN32_FIND_DATA wfd;
	HANDLE hFindFile;
	HCRYPTPROV hProv;
	LARGE_INTEGER lRealFileSize;
	BYTE pass1Data;
	BYTE pass2Data;
	BYTE pass3Data;
	DWORD nBytesWritten;
	BOOL bFolder;
	int i = 0;

	hFindFile = FindFirstFile(strFilename.c_str(), &wfd);

	if (hFindFile == INVALID_HANDLE_VALUE)
	{
		return;
	}

	bFolder = (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;

	FindClose(hFindFile);

	if (bFolder)
	{
		return;
	}

	/* Determine the actual size of the file on disk
	(i.e. how many clusters it is allocated). */
	GetFileClusterSize(strFilename, &lRealFileSize);

	/* Open the file, block any sharing mode, to stop the file
	been opened while it is overwritten. */
	hFile =
		CreateFile(strFilename.c_str(), FILE_WRITE_DATA, 0, nullptr, OPEN_EXISTING, NULL, nullptr);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		return;
	}

	/* Extend the file out to the end of its last sector. */
	SetFilePointerEx(hFile, lRealFileSize, nullptr, FILE_BEGIN);
	SetEndOfFile(hFile);

	/* Start at the beginning of the file, and
	write in the first-pass data, 0x00 over
	the length of the whole file. */
	SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);
	pass1Data = 0x00;

	for (i = 0; i < lRealFileSize.QuadPart; i++)
	{
		WriteFile(hFile, (LPVOID) &pass1Data, 1, &nBytesWritten, nullptr);
	}

	if (overwriteMethod == OverwriteMethod::ThreePass)
	{
		/* Start at the beginning of the file, and
		write in the second-pass data, 0xFF over
		the length of the whole file. */
		SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);
		pass2Data = 0xFF;

		for (i = 0; i < lRealFileSize.QuadPart; i++)
		{
			WriteFile(hFile, (LPVOID) &pass2Data, 1, &nBytesWritten, nullptr);
		}

		SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);

		CryptAcquireContext(&hProv, _T("SecureDelete"), nullptr, PROV_RSA_AES, CRYPT_NEWKEYSET);

		for (i = 0; i < lRealFileSize.QuadPart; i++)
		{
			CryptGenRandom(hProv, 1, (LPBYTE) &pass3Data);
			WriteFile(hFile, (LPVOID) &pass3Data, 1, &nBytesWritten, nullptr);
		}

		CryptAcquireContext(&hProv, _T("SecureDelete"), nullptr, PROV_RSA_AES, CRYPT_DELETEKEYSET);
	}

	FlushFileBuffers(hFile);

	CloseHandle(hFile);

	DeleteFile(strFilename.c_str());
}

HRESULT FileOperations::Undelete(const PCIDLIST_ABSOLUTE &pidl)
{
	wil::com_ptr_nothrow<IShellFolder> pDesktop;
	HRESULT hr = SHGetDesktopFolder(&pDesktop);
	RETURN_IF_FAILED(SHGetDesktopFolder(&pDesktop));

	PidlAbsolute pidlBin;
	hr = SHGetKnownFolderIDList(FOLDERID_RecycleBinFolder, KF_FLAG_DEFAULT, nullptr, PidlOutParam(pidlBin));
	if (FAILED(hr) || !pidlBin.Raw())
		return hr;

	wil::com_ptr_nothrow<IShellFolder> pShellFolder{};
	hr = pDesktop->BindToObject(pidlBin.Raw(), nullptr, IID_PPV_ARGS(&pShellFolder));
	if (FAILED(hr) || !pShellFolder)
		return hr;

	wil::com_ptr_nothrow<IEnumIDList> enumerator{};
	hr = pShellFolder->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &enumerator);
	if (FAILED(hr) || !enumerator)
		return hr;

	ULONG numFetched = 1;
	unique_pidl_child pidlItem;
	while (S_OK == enumerator->Next(1, wil::out_param(pidlItem), &numFetched) && (1 == numFetched))
	{
		const auto pidlRelative = ILFindLastID(static_cast<PCUIDLIST_RELATIVE>(pidl));
		hr = pShellFolder->CompareIDs(SHCIDS_CANONICALONLY, pidlRelative, pidlItem.get());
		if (0 == static_cast<short>(HRESULT_CODE(hr)))
		{
			hr = PerformUndeleting(pShellFolder, pidlItem.get());
			break;
		}
	}

	return hr;
}

HRESULT FileOperations::PerformUndeleting(wil::com_ptr_nothrow<IShellFolder> &shellFolder,
	const PITEMID_CHILD &pidChild)
{
	PITEMID_CHILD *item = static_cast<PITEMID_CHILD *>(CoTaskMemAlloc(sizeof(PITEMID_CHILD)));
	SecureZeroMemory(item, sizeof(PITEMID_CHILD));
	item[0] = pidChild;

	wil::com_ptr_nothrow<IContextMenu> pContextMenu{};
	HRESULT hr =
		shellFolder->GetUIObjectOf(nullptr, 1, reinterpret_cast<PCUITEMID_CHILD_ARRAY>(item),
			__uuidof(IContextMenu), nullptr, reinterpret_cast<void **>(&pContextMenu));
	if (SUCCEEDED(hr) && pContextMenu)
		hr = InvokeVerb(pContextMenu.get(), "undelete");

	CoTaskMemFree(item);

	return hr;
}

HRESULT FileOperations::InvokeVerb(IContextMenu* pContextMenu, PCSTR pszVerb)
{
	HRESULT hr{};
	const HMENU hmenu = CreatePopupMenu();
	if (pContextMenu && hmenu)
	{
		hr = pContextMenu->QueryContextMenu(hmenu, 0, 1, 0x7FFF, CMF_NORMAL);
		if (SUCCEEDED(hr))
		{
			CMINVOKECOMMANDINFO info = { 0 };
			info.cbSize = sizeof(info);
			info.lpVerb = pszVerb;
			hr = pContextMenu->InvokeCommand(&info);
		}
		DestroyMenu(hmenu);
	}
	return hr;
}

