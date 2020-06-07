// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FileOperations.h"
#include "DriveInfo.h"
#include "Helper.h"
#include "Macros.h"
#include "ShellHelper.h"
#include "StringHelper.h"
#include "iDataObject.h"
#include <boost/scope_exit.hpp>
#include <list>
#include <sstream>

#pragma warning(                                                                                   \
	disable : 4459) // declaration of 'boost_scope_exit_aux_args' hides global declaration

enum class PasteType
{
	Link,
	HardLink
};

int PasteFilesFromClipboardSpecial(const TCHAR *szDestination, PasteType pasteType);
BOOL GetFileClusterSize(const std::wstring &strFilename, PLARGE_INTEGER lpRealFileSize);

HRESULT NFileOperations::RenameFile(IShellItem *item, const std::wstring &newName)
{
	IFileOperation *fo;
	HRESULT hr = CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&fo));

	if (FAILED(hr))
	{
		return hr;
	}

	BOOST_SCOPE_EXIT(fo)
	{
		fo->Release();
	}
	BOOST_SCOPE_EXIT_END

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

HRESULT NFileOperations::DeleteFiles(
	HWND hwnd, std::vector<PCIDLIST_ABSOLUTE> &pidls, bool permanent, bool silent)
{
	IFileOperation *fo;
	HRESULT hr = CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&fo));

	if (FAILED(hr))
	{
		return hr;
	}

	BOOST_SCOPE_EXIT(fo)
	{
		fo->Release();
	}
	BOOST_SCOPE_EXIT_END

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

	IShellItemArray *shellItemArray;
	hr = SHCreateShellItemArrayFromIDLists(
		static_cast<UINT>(pidls.size()), &pidls[0], &shellItemArray);

	if (FAILED(hr))
	{
		return hr;
	}

	BOOST_SCOPE_EXIT(shellItemArray)
	{
		shellItemArray->Release();
	}
	BOOST_SCOPE_EXIT_END

	IUnknown *unknown;
	hr = shellItemArray->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&unknown));

	if (FAILED(hr))
	{
		return hr;
	}

	BOOST_SCOPE_EXIT(unknown)
	{
		unknown->Release();
	}
	BOOST_SCOPE_EXIT_END

	hr = fo->DeleteItems(unknown);

	if (FAILED(hr))
	{
		return hr;
	}

	hr = fo->PerformOperations();

	return hr;
}

HRESULT NFileOperations::CopyFilesToFolder(
	HWND hOwner, const std::wstring &strTitle, std::vector<PCIDLIST_ABSOLUTE> &pidls, bool move)
{
	PIDLIST_ABSOLUTE pidl;
	BOOL bRes = NFileOperations::CreateBrowseDialog(hOwner, strTitle, &pidl);

	if (!bRes)
	{
		return E_FAIL;
	}

	BOOST_SCOPE_EXIT(pidl)
	{
		CoTaskMemFree(pidl);
	}
	BOOST_SCOPE_EXIT_END

	IShellItem *destinationFolder = nullptr;
	HRESULT hr = SHCreateItemFromIDList(pidl, IID_PPV_ARGS(&destinationFolder));

	if (FAILED(hr))
	{
		return E_FAIL;
	}

	BOOST_SCOPE_EXIT(destinationFolder)
	{
		destinationFolder->Release();
	}
	BOOST_SCOPE_EXIT_END

	hr = CopyFiles(hOwner, destinationFolder, pidls, move);

	return hr;
}

HRESULT NFileOperations::CopyFiles(
	HWND hwnd, IShellItem *destinationFolder, std::vector<PCIDLIST_ABSOLUTE> &pidls, bool move)
{
	IFileOperation *fo;
	HRESULT hr = CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&fo));

	if (FAILED(hr))
	{
		return hr;
	}

	BOOST_SCOPE_EXIT(fo)
	{
		fo->Release();
	}
	BOOST_SCOPE_EXIT_END

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

	IShellItemArray *shellItemArray;
	hr = SHCreateShellItemArrayFromIDLists(
		static_cast<UINT>(pidls.size()), &pidls[0], &shellItemArray);

	if (FAILED(hr))
	{
		return hr;
	}

	BOOST_SCOPE_EXIT(shellItemArray)
	{
		shellItemArray->Release();
	}
	BOOST_SCOPE_EXIT_END

	IUnknown *unknown;
	hr = shellItemArray->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&unknown));

	if (FAILED(hr))
	{
		return hr;
	}

	BOOST_SCOPE_EXIT(unknown)
	{
		unknown->Release();
	}
	BOOST_SCOPE_EXIT_END

	if (move)
	{
		hr = fo->MoveItems(unknown, destinationFolder);
	}
	else
	{
		hr = fo->CopyItems(unknown, destinationFolder);
	}

	if (FAILED(hr))
	{
		return hr;
	}

	hr = fo->PerformOperations();

	return hr;
}

TCHAR *NFileOperations::BuildFilenameList(const std::list<std::wstring> &FilenameList)
{
	TCHAR *pszFilenames = NULL;
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
HRESULT NFileOperations::CreateNewFolder(IShellItem *destinationFolder,
	const std::wstring &newFolderName, IFileOperationProgressSink *progressSink)
{
	IFileOperation *fo;
	HRESULT hr = CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&fo));

	if (FAILED(hr))
	{
		return hr;
	}

	BOOST_SCOPE_EXIT(fo)
	{
		fo->Release();
	}
	BOOST_SCOPE_EXIT_END

	hr = fo->SetOperationFlags(FOF_ALLOWUNDO | FOF_SILENT);

	if (FAILED(hr))
	{
		return hr;
	}

	hr = fo->NewItem(
		destinationFolder, FILE_ATTRIBUTE_DIRECTORY, newFolderName.c_str(), nullptr, progressSink);

	if (FAILED(hr))
	{
		return hr;
	}

	hr = fo->PerformOperations();

	return hr;
}

BOOL NFileOperations::SaveDirectoryListing(
	const std::wstring &strDirectory, const std::wstring &strFilename)
{
	std::wstring strContents = _T("Directory\r\n---------\r\n") + strDirectory + _T("\r\n\r\n");

	SYSTEMTIME st;
	FILETIME ft;
	FILETIME lft;
	GetLocalTime(&st);
	SystemTimeToFileTime(&st, &ft);
	LocalFileTimeToFileTime(&ft, &lft);

	TCHAR szTime[128];
	CreateFileTimeString(&lft, szTime, SIZEOF_ARRAY(szTime), FALSE);
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

	TCHAR szTotalSize[32];
	FormatSizeString(ulTotalSize, szTotalSize, SIZEOF_ARRAY(szTotalSize));
	strContents += _T("Total size (not including subfolders): ") + std::wstring(szTotalSize)
		+ std::wstring(_T("\r\n"));

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

	HANDLE hFile = CreateFile(
		strFilename.c_str(), FILE_WRITE_DATA, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD nBytesWritten;

		/* Write out the BOM for UTF-16 LE data.
		See http://en.wikipedia.org/wiki/Byte-order_mark */
		WriteFile(hFile, reinterpret_cast<LPCVOID>("\xFF\xFE"), 2, &nBytesWritten, NULL);

		WriteFile(hFile, reinterpret_cast<LPCVOID>(strContents.c_str()),
			static_cast<DWORD>(strContents.size() * sizeof(WCHAR)), &nBytesWritten, NULL);

		if (nBytesWritten == strContents.size())
		{
			return TRUE;
		}

		CloseHandle(hFile);
	}

	return FALSE;
}

HRESULT CopyFiles(const std::list<std::wstring> &FileNameList, IDataObject **pClipboardDataObject)
{
	return CopyFilesToClipboard(FileNameList, FALSE, pClipboardDataObject);
}

HRESULT CutFiles(const std::list<std::wstring> &FileNameList, IDataObject **pClipboardDataObject)
{
	return CopyFilesToClipboard(FileNameList, TRUE, pClipboardDataObject);
}

HRESULT CopyFilesToClipboard(
	const std::list<std::wstring> &FileNameList, BOOL bMove, IDataObject **pClipboardDataObject)
{
	FORMATETC ftc[2];
	STGMEDIUM stg[2];
	BuildHDropList(&ftc[0], &stg[0], FileNameList);

	ftc[1].cfFormat = (CLIPFORMAT) RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
	ftc[1].ptd = NULL;
	ftc[1].dwAspect = DVASPECT_CONTENT;
	ftc[1].lindex = -1;
	ftc[1].tymed = TYMED_HGLOBAL;

	HRESULT hr = E_FAIL;
	HGLOBAL hglb = GlobalAlloc(GMEM_MOVEABLE, sizeof(DWORD));

	if (hglb != NULL)
	{
		auto *pdwCopyEffect = static_cast<DWORD *>(GlobalLock(hglb));

		if (pdwCopyEffect != NULL)
		{
			if (bMove)
			{
				*pdwCopyEffect = DROPEFFECT_MOVE;
			}
			else
			{
				*pdwCopyEffect = DROPEFFECT_COPY;
			}

			GlobalUnlock(hglb);

			stg[1].pUnkForRelease = NULL;
			stg[1].hGlobal = hglb;
			stg[1].tymed = TYMED_HGLOBAL;

			*pClipboardDataObject = CreateDataObject(ftc, stg, 2);

			IDataObjectAsyncCapability *pAsyncCapability = NULL;
			hr = (*pClipboardDataObject)->QueryInterface(IID_PPV_ARGS(&pAsyncCapability));

			if (SUCCEEDED(hr))
			{
				pAsyncCapability->SetAsyncMode(TRUE);
				pAsyncCapability->Release();

				hr = OleSetClipboard(*pClipboardDataObject);
			}
		}
	}

	return hr;
}

int PasteLinksToClipboardFiles(const TCHAR *szDestination)
{
	return PasteFilesFromClipboardSpecial(szDestination, PasteType::Link);
}

int PasteHardLinks(const TCHAR *szDestination)
{
	return PasteFilesFromClipboardSpecial(szDestination, PasteType::HardLink);
}

/* TODO: Use CDropHandler. */
int PasteFilesFromClipboardSpecial(const TCHAR *szDestination, PasteType pasteType)
{
	IDataObject *clipboardObject = NULL;
	DROPFILES *pdf = NULL;
	FORMATETC ftc;
	STGMEDIUM stg;
	HRESULT hr;
	TCHAR szFileName[MAX_PATH];
	TCHAR szLinkFileName[MAX_PATH];
	TCHAR szOldFileName[MAX_PATH];
	int nFilesCopied = -1;
	int i = 0;

	hr = OleGetClipboard(&clipboardObject);

	if (SUCCEEDED(hr))
	{
		ftc.cfFormat = CF_HDROP;
		ftc.ptd = NULL;
		ftc.dwAspect = DVASPECT_CONTENT;
		ftc.lindex = -1;
		ftc.tymed = TYMED_HGLOBAL;

		hr = clipboardObject->GetData(&ftc, &stg);

		if (SUCCEEDED(hr))
		{
			pdf = (DROPFILES *) GlobalLock(stg.hGlobal);

			if (pdf != NULL)
			{
				nFilesCopied = DragQueryFile((HDROP) pdf, 0xFFFFFFFF, NULL, 0);

				for (i = 0; i < nFilesCopied; i++)
				{
					DragQueryFile((HDROP) pdf, i, szOldFileName, SIZEOF_ARRAY(szOldFileName));

					StringCchCopy(szLinkFileName, SIZEOF_ARRAY(szLinkFileName), szDestination);

					StringCchCopy(szFileName, SIZEOF_ARRAY(szFileName), szOldFileName);
					PathStripPath(szFileName);

					PathAppend(szLinkFileName, szFileName);

					switch (pasteType)
					{
					case PasteType::Link:
						PathRenameExtension(szLinkFileName, _T(".lnk"));
						NFileOperations::CreateLinkToFile(
							szOldFileName, szLinkFileName, EMPTY_STRING);
						break;

					case PasteType::HardLink:
						CreateHardLink(szLinkFileName, szOldFileName, NULL);
						break;
					}
				}

				GlobalUnlock(stg.hGlobal);
			}

			ReleaseStgMedium(&stg);
		}
		clipboardObject->Release();
	}

	return nFilesCopied;
}

HRESULT NFileOperations::CreateLinkToFile(const std::wstring &strTargetFilename,
	const std::wstring &strLinkFilename, const std::wstring &strLinkDescription)
{
	IShellLink *pShellLink = NULL;
	HRESULT hr =
		CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pShellLink));

	if (SUCCEEDED(hr))
	{
		pShellLink->SetPath(strTargetFilename.c_str());
		pShellLink->SetDescription(strLinkDescription.c_str());

		IPersistFile *pPersistFile = NULL;
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

HRESULT NFileOperations::ResolveLink(
	HWND hwnd, DWORD fFlags, const TCHAR *szLinkFilename, TCHAR *szResolvedPath, int nBufferSize)
{
	SHFILEINFO shfi;
	DWORD_PTR dwRet = SHGetFileInfo(szLinkFilename, NULL, &shfi, sizeof(shfi), SHGFI_ATTRIBUTES);

	if (dwRet == 0 || !(shfi.dwAttributes & SFGAO_LINK))
	{
		return E_FAIL;
	}

	IShellLink *pShellLink = NULL;
	HRESULT hr =
		CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pShellLink));

	if (hr == S_OK)
	{
		IPersistFile *pPersistFile = NULL;
		hr = pShellLink->QueryInterface(IID_PPV_ARGS(&pPersistFile));

		if (hr == S_OK)
		{
			hr = pPersistFile->Load(szLinkFilename, STGM_READ);

			if (hr == S_OK)
			{
				pShellLink->Resolve(hwnd, fFlags);

				TCHAR szResolvedPathInternal[MAX_PATH];
				pShellLink->GetPath(szResolvedPathInternal, SIZEOF_ARRAY(szResolvedPathInternal),
					NULL, SLGP_UNCPRIORITY);

				StringCchCopy(szResolvedPath, nBufferSize, szResolvedPathInternal);
			}

			pPersistFile->Release();
		}

		pShellLink->Release();
	}

	return hr;
}

BOOL NFileOperations::CreateBrowseDialog(
	HWND hOwner, const std::wstring &strTitle, PIDLIST_ABSOLUTE *ppidl)
{
	TCHAR szDisplayName[MAX_PATH];

	BROWSEINFO bi;
	bi.hwndOwner = hOwner;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = szDisplayName;
	bi.lpszTitle = strTitle.c_str();
	bi.ulFlags = BIF_NEWDIALOGSTYLE;
	bi.lpfn = NULL;
	*ppidl = SHBrowseForFolder(&bi);

	BOOL bSuccessful = (*ppidl != NULL);

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
	HRESULT hr = StringCchCopy(szRoot, SIZEOF_ARRAY(szRoot), strFilename.c_str());

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

void NFileOperations::DeleteFileSecurely(
	const std::wstring &strFilename, OverwriteMethod overwriteMethod)
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
	hFile = CreateFile(strFilename.c_str(), FILE_WRITE_DATA, 0, NULL, OPEN_EXISTING, NULL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		return;
	}

	/* Extend the file out to the end of its last sector. */
	SetFilePointerEx(hFile, lRealFileSize, NULL, FILE_BEGIN);
	SetEndOfFile(hFile);

	/* Start at the beginning of the file, and
	write in the first-pass data, 0x00 over
	the length of the whole file. */
	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
	pass1Data = 0x00;

	for (i = 0; i < lRealFileSize.QuadPart; i++)
	{
		WriteFile(hFile, (LPVOID) &pass1Data, 1, &nBytesWritten, NULL);
	}

	if (overwriteMethod == OverwriteMethod::ThreePass)
	{
		/* Start at the beginning of the file, and
		write in the second-pass data, 0xFF over
		the length of the whole file. */
		SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
		pass2Data = 0xFF;

		for (i = 0; i < lRealFileSize.QuadPart; i++)
		{
			WriteFile(hFile, (LPVOID) &pass2Data, 1, &nBytesWritten, NULL);
		}

		SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

		CryptAcquireContext(&hProv, _T("SecureDelete"), NULL, PROV_RSA_AES, CRYPT_NEWKEYSET);

		for (i = 0; i < lRealFileSize.QuadPart; i++)
		{
			CryptGenRandom(hProv, 1, (LPBYTE) &pass3Data);
			WriteFile(hFile, (LPVOID) &pass3Data, 1, &nBytesWritten, NULL);
		}

		CryptAcquireContext(&hProv, _T("SecureDelete"), NULL, PROV_RSA_AES, CRYPT_DELETEKEYSET);
	}

	FlushFileBuffers(hFile);

	CloseHandle(hFile);

	DeleteFile(strFilename.c_str());
}