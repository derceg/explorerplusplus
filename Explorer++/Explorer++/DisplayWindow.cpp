// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "DisplayWindow/DisplayWindow.h"
#include "MainResource.h"
#include "ShellBrowser/ShellBrowser.h"
#include "TabContainer.h"
#include "../Helper/FolderSize.h"
#include "../Helper/Helper.h"
#include "../Helper/ShellHelper.h"

void Explorerplusplus::UpdateDisplayWindow(const Tab &tab)
{
	DisplayWindow_ClearTextBuffer(m_hDisplayWindow);

	int nSelected = tab.GetShellBrowser()->GetNumSelected();

	if (nSelected == 0)
	{
		UpdateDisplayWindowForZeroFiles(tab);
	}
	else if (nSelected == 1)
	{
		UpdateDisplayWindowForOneFile(tab);
	}
	else if (nSelected > 1)
	{
		UpdateDisplayWindowForMultipleFiles(tab);
	}
}

void Explorerplusplus::UpdateDisplayWindowForZeroFiles(const Tab &tab)
{
	/* Clear out any previous data shown in the display window. */
	DisplayWindow_ClearTextBuffer(m_hDisplayWindow);
	DisplayWindow_SetThumbnailFile(m_hDisplayWindow, EMPTY_STRING, FALSE);

	std::wstring currentDirectory = tab.GetShellBrowser()->GetDirectory();
	auto pidlDirectory = tab.GetShellBrowser()->GetDirectoryIdl();

	unique_pidl_absolute pidlComputer;
	SHGetFolderLocation(nullptr, CSIDL_DRIVES, nullptr, 0, wil::out_param(pidlComputer));

	if (ArePidlsEquivalent(pidlDirectory.get(), pidlComputer.get()))
	{
		TCHAR szDisplay[512];
		DWORD dwSize = SIZEOF_ARRAY(szDisplay);
		GetComputerName(szDisplay, &dwSize);
		DisplayWindow_BufferText(m_hDisplayWindow, szDisplay);

		std::wstring cpuBrand;
		TCHAR szTemp[512];
		HRESULT hr = GetCPUBrandString(cpuBrand);

		if (SUCCEEDED(hr))
		{
			LoadString(m_resourceInstance, IDS_GENERAL_DISPLAY_WINDOW_PROCESSOR, szTemp,
				SIZEOF_ARRAY(szTemp));
			StringCchPrintf(szDisplay, SIZEOF_ARRAY(szDisplay), szTemp, cpuBrand.c_str());
			DisplayWindow_BufferText(m_hDisplayWindow, szDisplay);
		}

		MEMORYSTATUSEX memoryStatus = {};
		memoryStatus.dwLength = sizeof(memoryStatus);
		GlobalMemoryStatusEx(&memoryStatus);

		auto memorySizeText = FormatSizeString(memoryStatus.ullTotalPhys);
		LoadString(m_resourceInstance, IDS_GENERAL_DISPLAY_WINDOW_MEMORY, szTemp,
			SIZEOF_ARRAY(szTemp));
		StringCchPrintf(szDisplay, SIZEOF_ARRAY(szDisplay), szTemp, memorySizeText.c_str());
		DisplayWindow_BufferText(m_hDisplayWindow, szDisplay);
	}
	else
	{
		/* Folder name. */
		std::wstring folderName;
		GetDisplayName(currentDirectory.c_str(), SHGDN_INFOLDER, folderName);
		DisplayWindow_BufferText(m_hDisplayWindow, folderName.c_str());

		/* Folder type. */
		SHFILEINFO shfi;
		SHGetFileInfo(reinterpret_cast<LPCTSTR>(pidlDirectory.get()), 0, &shfi, sizeof(shfi),
			SHGFI_PIDL | SHGFI_TYPENAME);
		DisplayWindow_BufferText(m_hDisplayWindow, shfi.szTypeName);
	}
}

void Explorerplusplus::UpdateDisplayWindowForOneFile(const Tab &tab)
{
	WIN32_FIND_DATA wfd;
	SHFILEINFO shfi;
	TCHAR szFileDate[256];
	TCHAR szDisplayDate[512];
	TCHAR szDateModified[256];
	int iSelected;

	iSelected = ListView_GetNextItem(m_hActiveListView, -1, LVNI_SELECTED);

	if (iSelected != -1)
	{
		std::wstring filename = tab.GetShellBrowser()->GetItemName(iSelected);

		/* File name. */
		DisplayWindow_BufferText(m_hDisplayWindow, filename.c_str());

		std::wstring fullItemName = tab.GetShellBrowser()->GetItemFullName(iSelected);

		if (!tab.GetShellBrowser()->InVirtualFolder())
		{
			DWORD dwAttributes;

			wfd = tab.GetShellBrowser()->GetItemFileFindData(iSelected);

			dwAttributes = GetFileAttributes(fullItemName.c_str());

			if (((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
				&& m_config->globalFolderSettings.showFolderSizes)
			{
				FolderSize_t *pfs = nullptr;
				FolderSizeExtraInfo *pfsei = nullptr;
				DWFolderSize displayWindowFolderSize;
				TCHAR szDisplayText[256];
				TCHAR szTotalSize[64];
				TCHAR szCalculating[64];
				DWORD threadId;

				pfs = (FolderSize_t *) malloc(sizeof(FolderSize_t));

				if (pfs != nullptr)
				{
					pfsei = (FolderSizeExtraInfo *) malloc(sizeof(FolderSizeExtraInfo));

					if (pfsei != nullptr)
					{
						pfsei->pContainer = (void *) this;
						pfsei->uId = m_iDWFolderSizeUniqueId;
						pfs->pData = (LPVOID) pfsei;

						pfs->pfnCallback = FolderSizeCallbackStub;

						StringCchCopy(pfs->szPath, SIZEOF_ARRAY(pfs->szPath), fullItemName.c_str());

						LoadString(m_resourceInstance, IDS_GENERAL_TOTALSIZE, szTotalSize,
							SIZEOF_ARRAY(szTotalSize));
						LoadString(m_resourceInstance, IDS_GENERAL_CALCULATING, szCalculating,
							SIZEOF_ARRAY(szCalculating));
						StringCchPrintf(szDisplayText, SIZEOF_ARRAY(szDisplayText), _T("%s: %s"),
							szTotalSize, szCalculating);
						DisplayWindow_BufferText(m_hDisplayWindow, szDisplayText);

						/* Maintain a global list of folder size operations. */
						displayWindowFolderSize.uId = m_iDWFolderSizeUniqueId;
						displayWindowFolderSize.iTabId =
							GetActivePane()->GetTabContainer()->GetSelectedTab().GetId();
						displayWindowFolderSize.bValid = TRUE;
						m_DWFolderSizes.push_back(displayWindowFolderSize);

						HANDLE hThread = CreateThread(nullptr, 0, Thread_CalculateFolderSize,
							(LPVOID) pfs, 0, &threadId);
						CloseHandle(hThread);

						m_iDWFolderSizeUniqueId++;
					}
					else
					{
						free(pfs);
					}
				}
			}
			else
			{
				SHGetFileInfo(fullItemName.c_str(), wfd.dwFileAttributes, &shfi, sizeof(shfi),
					SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES);

				DisplayWindow_BufferText(m_hDisplayWindow, shfi.szTypeName);
			}

			CreateFileTimeString(&wfd.ftLastWriteTime, szFileDate, SIZEOF_ARRAY(szFileDate),
				m_config->globalFolderSettings.showFriendlyDates);

			LoadString(m_resourceInstance, IDS_GENERAL_DATEMODIFIED, szDateModified,
				SIZEOF_ARRAY(szDateModified));

			StringCchPrintf(szDisplayDate, SIZEOF_ARRAY(szDisplayDate), _T("%s: %s"),
				szDateModified, szFileDate);

			/* File (modified) date. */
			DisplayWindow_BufferText(m_hDisplayWindow, szDisplayDate);

			if (IsImage(fullItemName.c_str()))
			{
				TCHAR szOutput[256];
				TCHAR szTemp[64];
				UINT uWidth;
				UINT uHeight;
				Gdiplus::Image *pimg = nullptr;

				pimg = new Gdiplus::Image(fullItemName.c_str(), FALSE);

				if (pimg->GetLastStatus() == Gdiplus::Ok)
				{
					uWidth = pimg->GetWidth();
					LoadString(m_resourceInstance, IDS_GENERAL_DISPLAYWINDOW_IMAGEWIDTH, szTemp,
						SIZEOF_ARRAY(szTemp));
					StringCchPrintf(szOutput, SIZEOF_ARRAY(szOutput), szTemp, uWidth);
					DisplayWindow_BufferText(m_hDisplayWindow, szOutput);

					uHeight = pimg->GetHeight();
					LoadString(m_resourceInstance, IDS_GENERAL_DISPLAYWINDOW_IMAGEHEIGHT, szTemp,
						SIZEOF_ARRAY(szTemp));
					StringCchPrintf(szOutput, SIZEOF_ARRAY(szOutput), szTemp, uHeight);
					DisplayWindow_BufferText(m_hDisplayWindow, szOutput);

					Gdiplus::PixelFormat format;
					UINT uBitDepth;

					format = pimg->GetPixelFormat();

					switch (format)
					{
					case PixelFormat1bppIndexed:
						uBitDepth = 1;
						break;

					case PixelFormat4bppIndexed:
						uBitDepth = 4;
						break;

					case PixelFormat8bppIndexed:
						uBitDepth = 8;
						break;

					case PixelFormat16bppARGB1555:
					case PixelFormat16bppGrayScale:
					case PixelFormat16bppRGB555:
					case PixelFormat16bppRGB565:
						uBitDepth = 16;
						break;

					case PixelFormat24bppRGB:
						uBitDepth = 24;
						break;

					case PixelFormat32bppARGB:
					case PixelFormat32bppPARGB:
					case PixelFormat32bppRGB:
						uBitDepth = 32;
						break;

					case PixelFormat48bppRGB:
						uBitDepth = 48;
						break;

					case PixelFormat64bppARGB:
					case PixelFormat64bppPARGB:
						uBitDepth = 64;
						break;

					default:
						uBitDepth = 0;
						break;
					}

					if (uBitDepth == 0)
					{
						LoadString(m_resourceInstance, IDS_GENERAL_DISPLAYWINDOW_BITDEPTHUNKNOWN,
							szTemp, SIZEOF_ARRAY(szTemp));
						StringCchCopy(szOutput, SIZEOF_ARRAY(szOutput), szTemp);
					}
					else
					{
						LoadString(m_resourceInstance, IDS_GENERAL_DISPLAYWINDOW_BITDEPTH, szTemp,
							SIZEOF_ARRAY(szTemp));
						StringCchPrintf(szOutput, SIZEOF_ARRAY(szOutput), szTemp, uBitDepth);
					}

					DisplayWindow_BufferText(m_hDisplayWindow, szOutput);

					Gdiplus::REAL res;

					res = pimg->GetHorizontalResolution();
					LoadString(m_resourceInstance, IDS_GENERAL_DISPLAYWINDOW_HORIZONTALRESOLUTION,
						szTemp, SIZEOF_ARRAY(szTemp));
					StringCchPrintf(szOutput, SIZEOF_ARRAY(szOutput), szTemp, res);
					DisplayWindow_BufferText(m_hDisplayWindow, szOutput);

					res = pimg->GetVerticalResolution();
					LoadString(m_resourceInstance, IDS_GENERAL_DISPLAYWINDOW_VERTICALRESOLUTION,
						szTemp, SIZEOF_ARRAY(szTemp));
					StringCchPrintf(szOutput, SIZEOF_ARRAY(szOutput), szTemp, res);
					DisplayWindow_BufferText(m_hDisplayWindow, szOutput);
				}

				delete pimg;
			}

			/* Only attempt to show file previews for files (not folders). Also, only
			attempt to show a preview if the display window is actually active. */
			if (((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
				&& m_config->showFilePreviews && m_config->showDisplayWindow)
			{
				DisplayWindow_SetThumbnailFile(m_hDisplayWindow, fullItemName.c_str(), TRUE);
			}
			else
			{
				DisplayWindow_SetThumbnailFile(m_hDisplayWindow, EMPTY_STRING, FALSE);
			}
		}
		else
		{
			if (PathIsRoot(fullItemName.c_str()))
			{
				TCHAR szMsg[64];
				TCHAR szTemp[64];
				ULARGE_INTEGER ulTotalNumberOfBytes;
				ULARGE_INTEGER ulTotalNumberOfFreeBytes;
				BOOL bRet = GetDiskFreeSpaceEx(fullItemName.c_str(), nullptr, &ulTotalNumberOfBytes,
					&ulTotalNumberOfFreeBytes);

				if (bRet)
				{
					auto sizeText = FormatSizeString(ulTotalNumberOfFreeBytes.QuadPart);
					LoadString(m_resourceInstance, IDS_GENERAL_DISPLAY_WINDOW_FREE_SPACE, szTemp,
						SIZEOF_ARRAY(szTemp));
					StringCchPrintf(szMsg, SIZEOF_ARRAY(szMsg), szTemp, sizeText.c_str());
					DisplayWindow_BufferText(m_hDisplayWindow, szMsg);

					sizeText = FormatSizeString(ulTotalNumberOfBytes.QuadPart);
					LoadString(m_resourceInstance, IDS_GENERAL_DISPLAY_WINDOW_TOTAL_SIZE, szTemp,
						SIZEOF_ARRAY(szTemp));
					StringCchPrintf(szMsg, SIZEOF_ARRAY(szMsg), szTemp, sizeText.c_str());
					DisplayWindow_BufferText(m_hDisplayWindow, szMsg);
				}

				TCHAR szFileSystem[MAX_PATH + 1];
				bRet = GetVolumeInformation(fullItemName.c_str(), nullptr, 0, nullptr, nullptr,
					nullptr, szFileSystem, SIZEOF_ARRAY(szFileSystem));

				if (bRet)
				{
					LoadString(m_resourceInstance, IDS_GENERAL_DISPLAY_WINDOW_FILE_SYSTEM, szTemp,
						SIZEOF_ARRAY(szTemp));
					StringCchPrintf(szMsg, SIZEOF_ARRAY(szMsg), szTemp, szFileSystem);
					DisplayWindow_BufferText(m_hDisplayWindow, szMsg);
				}
			}
		}
	}
}

void Explorerplusplus::UpdateDisplayWindowForMultipleFiles(const Tab &tab)
{
	TCHAR szNumSelected[64] = EMPTY_STRING;
	TCHAR szTotalSize[64] = EMPTY_STRING;
	TCHAR szMore[64];
	TCHAR szTotalSizeString[64];
	int nSelected;

	DisplayWindow_SetThumbnailFile(m_hDisplayWindow, EMPTY_STRING, FALSE);

	nSelected = tab.GetShellBrowser()->GetNumSelected();

	LoadString(m_resourceInstance, IDS_GENERAL_SELECTED_MULTIPLE_ITEMS, szMore,
		SIZEOF_ARRAY(szMore));

	StringCchPrintf(szNumSelected, SIZEOF_ARRAY(szNumSelected), _T("%d %s"), nSelected, szMore);

	DisplayWindow_BufferText(m_hDisplayWindow, szNumSelected);

	if (!tab.GetShellBrowser()->InVirtualFolder())
	{
		uint64_t selectionSize = tab.GetShellBrowser()->GetSelectionSize();
		auto displayFormat = m_config->globalFolderSettings.forceSize
			? m_config->globalFolderSettings.sizeDisplayFormat
			: +SizeDisplayFormat::None;
		auto selectionSizeText = FormatSizeString(selectionSize, displayFormat);

		LoadString(m_resourceInstance, IDS_GENERAL_TOTALFILESIZE, szTotalSizeString,
			SIZEOF_ARRAY(szTotalSizeString));

		StringCchPrintf(szTotalSize, SIZEOF_ARRAY(szTotalSize), _T("%s: %s"), szTotalSizeString,
			selectionSizeText.c_str());
	}

	DisplayWindow_BufferText(m_hDisplayWindow, szTotalSize);
}
