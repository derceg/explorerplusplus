// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "Config.h"
#include "DisplayWindow/DisplayWindow.h"
#include "MainResource.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "TabContainer.h"
#include "../Helper/FolderSize.h"
#include "../Helper/Helper.h"
#include "../Helper/ShellHelper.h"

void Explorerplusplus::UpdateDisplayWindow(const Tab &tab)
{
	DisplayWindow_ClearTextBuffer(m_displayWindow->GetHWND());

	int nSelected = tab.GetShellBrowserImpl()->GetNumSelected();

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
	DisplayWindow_ClearTextBuffer(m_displayWindow->GetHWND());
	DisplayWindow_SetThumbnailFile(m_displayWindow->GetHWND(), L"", FALSE);

	std::wstring currentDirectory = tab.GetShellBrowserImpl()->GetDirectoryPath();
	auto pidlDirectory = tab.GetShellBrowserImpl()->GetDirectoryIdl();

	unique_pidl_absolute pidlComputer;
	SHGetFolderLocation(nullptr, CSIDL_DRIVES, nullptr, 0, wil::out_param(pidlComputer));

	if (ArePidlsEquivalent(pidlDirectory.get(), pidlComputer.get()))
	{
		TCHAR szDisplay[512];
		DWORD dwSize = std::size(szDisplay);
		GetComputerName(szDisplay, &dwSize);
		DisplayWindow_BufferText(m_displayWindow->GetHWND(), szDisplay);

		std::wstring cpuBrand;
		TCHAR szTemp[512];
		HRESULT hr = GetCPUBrandString(cpuBrand);

		if (SUCCEEDED(hr))
		{
			LoadString(m_app->GetResourceInstance(), IDS_GENERAL_DISPLAY_WINDOW_PROCESSOR, szTemp,
				std::size(szTemp));
			StringCchPrintf(szDisplay, std::size(szDisplay), szTemp, cpuBrand.c_str());
			DisplayWindow_BufferText(m_displayWindow->GetHWND(), szDisplay);
		}

		MEMORYSTATUSEX memoryStatus = {};
		memoryStatus.dwLength = sizeof(memoryStatus);
		GlobalMemoryStatusEx(&memoryStatus);

		auto memorySizeText = FormatSizeString(memoryStatus.ullTotalPhys);
		LoadString(m_app->GetResourceInstance(), IDS_GENERAL_DISPLAY_WINDOW_MEMORY, szTemp,
			std::size(szTemp));
		StringCchPrintf(szDisplay, std::size(szDisplay), szTemp, memorySizeText.c_str());
		DisplayWindow_BufferText(m_displayWindow->GetHWND(), szDisplay);
	}
	else
	{
		/* Folder name. */
		std::wstring folderName;
		GetDisplayName(currentDirectory.c_str(), SHGDN_INFOLDER, folderName);
		DisplayWindow_BufferText(m_displayWindow->GetHWND(), folderName.c_str());

		/* Folder type. */
		SHFILEINFO shfi;
		SHGetFileInfo(reinterpret_cast<LPCTSTR>(pidlDirectory.get()), 0, &shfi, sizeof(shfi),
			SHGFI_PIDL | SHGFI_TYPENAME);
		DisplayWindow_BufferText(m_displayWindow->GetHWND(), shfi.szTypeName);
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
		std::wstring filename = tab.GetShellBrowserImpl()->GetItemName(iSelected);

		/* File name. */
		DisplayWindow_BufferText(m_displayWindow->GetHWND(), filename.c_str());

		std::wstring fullItemName = tab.GetShellBrowserImpl()->GetItemFullName(iSelected);

		if (!tab.GetShellBrowserImpl()->InVirtualFolder())
		{
			DWORD dwAttributes;

			wfd = tab.GetShellBrowserImpl()->GetItemFileFindData(iSelected);

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

						StringCchCopy(pfs->szPath, std::size(pfs->szPath), fullItemName.c_str());

						LoadString(m_app->GetResourceInstance(), IDS_GENERAL_TOTALSIZE, szTotalSize,
							std::size(szTotalSize));
						LoadString(m_app->GetResourceInstance(), IDS_GENERAL_CALCULATING,
							szCalculating, std::size(szCalculating));
						StringCchPrintf(szDisplayText, std::size(szDisplayText), _T("%s: %s"),
							szTotalSize, szCalculating);
						DisplayWindow_BufferText(m_displayWindow->GetHWND(), szDisplayText);

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

				DisplayWindow_BufferText(m_displayWindow->GetHWND(), shfi.szTypeName);
			}

			CreateFileTimeString(&wfd.ftLastWriteTime, szFileDate, std::size(szFileDate),
				m_config->globalFolderSettings.showFriendlyDates);

			LoadString(m_app->GetResourceInstance(), IDS_GENERAL_DATEMODIFIED, szDateModified,
				std::size(szDateModified));

			StringCchPrintf(szDisplayDate, std::size(szDisplayDate), _T("%s: %s"), szDateModified,
				szFileDate);

			/* File (modified) date. */
			DisplayWindow_BufferText(m_displayWindow->GetHWND(), szDisplayDate);

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
					LoadString(m_app->GetResourceInstance(), IDS_GENERAL_DISPLAYWINDOW_IMAGEWIDTH,
						szTemp, std::size(szTemp));
					StringCchPrintf(szOutput, std::size(szOutput), szTemp, uWidth);
					DisplayWindow_BufferText(m_displayWindow->GetHWND(), szOutput);

					uHeight = pimg->GetHeight();
					LoadString(m_app->GetResourceInstance(), IDS_GENERAL_DISPLAYWINDOW_IMAGEHEIGHT,
						szTemp, std::size(szTemp));
					StringCchPrintf(szOutput, std::size(szOutput), szTemp, uHeight);
					DisplayWindow_BufferText(m_displayWindow->GetHWND(), szOutput);

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
						LoadString(m_app->GetResourceInstance(),
							IDS_GENERAL_DISPLAYWINDOW_BITDEPTHUNKNOWN, szTemp, std::size(szTemp));
						StringCchCopy(szOutput, std::size(szOutput), szTemp);
					}
					else
					{
						LoadString(m_app->GetResourceInstance(), IDS_GENERAL_DISPLAYWINDOW_BITDEPTH,
							szTemp, std::size(szTemp));
						StringCchPrintf(szOutput, std::size(szOutput), szTemp, uBitDepth);
					}

					DisplayWindow_BufferText(m_displayWindow->GetHWND(), szOutput);

					Gdiplus::REAL res;

					res = pimg->GetHorizontalResolution();
					LoadString(m_app->GetResourceInstance(),
						IDS_GENERAL_DISPLAYWINDOW_HORIZONTALRESOLUTION, szTemp, std::size(szTemp));
					StringCchPrintf(szOutput, std::size(szOutput), szTemp, res);
					DisplayWindow_BufferText(m_displayWindow->GetHWND(), szOutput);

					res = pimg->GetVerticalResolution();
					LoadString(m_app->GetResourceInstance(),
						IDS_GENERAL_DISPLAYWINDOW_VERTICALRESOLUTION, szTemp, std::size(szTemp));
					StringCchPrintf(szOutput, std::size(szOutput), szTemp, res);
					DisplayWindow_BufferText(m_displayWindow->GetHWND(), szOutput);
				}

				delete pimg;
			}

			/* Only attempt to show file previews for files (not folders). Also, only
			attempt to show a preview if the display window is actually active. */
			if (((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
				&& m_config->showFilePreviews && m_config->showDisplayWindow.get())
			{
				DisplayWindow_SetThumbnailFile(m_displayWindow->GetHWND(), fullItemName.c_str(),
					TRUE);
			}
			else
			{
				DisplayWindow_SetThumbnailFile(m_displayWindow->GetHWND(), L"", FALSE);
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
					LoadString(m_app->GetResourceInstance(), IDS_GENERAL_DISPLAY_WINDOW_FREE_SPACE,
						szTemp, std::size(szTemp));
					StringCchPrintf(szMsg, std::size(szMsg), szTemp, sizeText.c_str());
					DisplayWindow_BufferText(m_displayWindow->GetHWND(), szMsg);

					sizeText = FormatSizeString(ulTotalNumberOfBytes.QuadPart);
					LoadString(m_app->GetResourceInstance(), IDS_GENERAL_DISPLAY_WINDOW_TOTAL_SIZE,
						szTemp, std::size(szTemp));
					StringCchPrintf(szMsg, std::size(szMsg), szTemp, sizeText.c_str());
					DisplayWindow_BufferText(m_displayWindow->GetHWND(), szMsg);
				}

				TCHAR szFileSystem[MAX_PATH + 1];
				bRet = GetVolumeInformation(fullItemName.c_str(), nullptr, 0, nullptr, nullptr,
					nullptr, szFileSystem, std::size(szFileSystem));

				if (bRet)
				{
					LoadString(m_app->GetResourceInstance(), IDS_GENERAL_DISPLAY_WINDOW_FILE_SYSTEM,
						szTemp, std::size(szTemp));
					StringCchPrintf(szMsg, std::size(szMsg), szTemp, szFileSystem);
					DisplayWindow_BufferText(m_displayWindow->GetHWND(), szMsg);
				}
			}
		}
	}
}

void Explorerplusplus::UpdateDisplayWindowForMultipleFiles(const Tab &tab)
{
	TCHAR szNumSelected[64] = L"";
	TCHAR szTotalSize[64] = L"";
	TCHAR szMore[64];
	TCHAR szTotalSizeString[64];
	int nSelected;

	DisplayWindow_SetThumbnailFile(m_displayWindow->GetHWND(), L"", FALSE);

	nSelected = tab.GetShellBrowserImpl()->GetNumSelected();

	LoadString(m_app->GetResourceInstance(), IDS_GENERAL_SELECTED_MULTIPLE_ITEMS, szMore,
		std::size(szMore));

	StringCchPrintf(szNumSelected, std::size(szNumSelected), _T("%d %s"), nSelected, szMore);

	DisplayWindow_BufferText(m_displayWindow->GetHWND(), szNumSelected);

	if (!tab.GetShellBrowserImpl()->InVirtualFolder())
	{
		uint64_t selectionSize = tab.GetShellBrowserImpl()->GetSelectionSize();
		auto displayFormat = m_config->globalFolderSettings.forceSize
			? m_config->globalFolderSettings.sizeDisplayFormat
			: +SizeDisplayFormat::None;
		auto selectionSizeText = FormatSizeString(selectionSize, displayFormat);

		LoadString(m_app->GetResourceInstance(), IDS_GENERAL_TOTALFILESIZE, szTotalSizeString,
			std::size(szTotalSizeString));

		StringCchPrintf(szTotalSize, std::size(szTotalSize), _T("%s: %s"), szTotalSizeString,
			selectionSizeText.c_str());
	}

	DisplayWindow_BufferText(m_displayWindow->GetHWND(), szTotalSize);
}
