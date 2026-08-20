// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "DisplayWindow/DisplayWindow.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "TabContainer.h"
#include "../Helper/FolderSize.h"
#include "../Helper/Helper.h"
#include "../Helper/ShellHelper.h"
#include <Shlwapi.h>

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
	const auto &pidlDirectory = tab.GetShellBrowser()->GetDirectory();

	unique_pidl_absolute pidlComputer;
	SHGetFolderLocation(nullptr, CSIDL_DRIVES, nullptr, 0, wil::out_param(pidlComputer));

	if (ArePidlsEquivalent(pidlDirectory.Raw(), pidlComputer.get()))
	{
		TCHAR szDisplay[512];
		DWORD dwSize = std::size(szDisplay);
		GetComputerName(szDisplay, &dwSize);
		DisplayWindow_BufferText(m_displayWindow->GetHWND(), szDisplay);

		std::wstring cpuBrand;
		HRESULT hr = GetCPUBrandString(cpuBrand);

		if (SUCCEEDED(hr))
		{
			auto processorTemplate =
				m_resourceLoader->LoadString(IDS_GENERAL_DISPLAY_WINDOW_PROCESSOR);
			StringCchPrintf(szDisplay, std::size(szDisplay), processorTemplate.c_str(),
				cpuBrand.c_str());
			DisplayWindow_BufferText(m_displayWindow->GetHWND(), szDisplay);
		}

		MEMORYSTATUSEX memoryStatus = {};
		memoryStatus.dwLength = sizeof(memoryStatus);
		GlobalMemoryStatusEx(&memoryStatus);

		auto memorySizeText = FormatSizeString(memoryStatus.ullTotalPhys);
		auto memoryTemplate = m_resourceLoader->LoadString(IDS_GENERAL_DISPLAY_WINDOW_MEMORY);
		StringCchPrintf(szDisplay, std::size(szDisplay), memoryTemplate.c_str(),
			memorySizeText.c_str());
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
		SHGetFileInfo(reinterpret_cast<LPCTSTR>(pidlDirectory.Raw()), 0, &shfi, sizeof(shfi),
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

						auto totalSizeText = m_resourceLoader->LoadString(IDS_GENERAL_TOTALSIZE);
						auto calculatingText =
							m_resourceLoader->LoadString(IDS_GENERAL_CALCULATING);
						StringCchPrintf(szDisplayText, std::size(szDisplayText), _T("%s: %s"),
							totalSizeText.c_str(), calculatingText.c_str());
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

			auto dateModifiedText = m_resourceLoader->LoadString(IDS_GENERAL_DATEMODIFIED);
			StringCchPrintf(szDisplayDate, std::size(szDisplayDate), _T("%s: %s"),
				dateModifiedText.c_str(), szFileDate);

			/* File (modified) date. */
			DisplayWindow_BufferText(m_displayWindow->GetHWND(), szDisplayDate);

			if (IsImage(fullItemName.c_str()))
			{
				TCHAR szOutput[256];
				UINT uWidth;
				UINT uHeight;
				Gdiplus::Image *pimg = nullptr;

				pimg = new Gdiplus::Image(fullItemName.c_str(), FALSE);

				if (pimg->GetLastStatus() == Gdiplus::Ok)
				{
					uWidth = pimg->GetWidth();
					auto imageWidthTemplate =
						m_resourceLoader->LoadString(IDS_GENERAL_DISPLAYWINDOW_IMAGEWIDTH);
					StringCchPrintf(szOutput, std::size(szOutput), imageWidthTemplate.c_str(),
						uWidth);
					DisplayWindow_BufferText(m_displayWindow->GetHWND(), szOutput);

					uHeight = pimg->GetHeight();
					auto imageHeightTemplate =
						m_resourceLoader->LoadString(IDS_GENERAL_DISPLAYWINDOW_IMAGEHEIGHT);
					StringCchPrintf(szOutput, std::size(szOutput), imageHeightTemplate.c_str(),
						uHeight);
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
						auto bitDepthUnknownText =
							m_resourceLoader->LoadString(IDS_GENERAL_DISPLAYWINDOW_BITDEPTHUNKNOWN);
						StringCchCopy(szOutput, std::size(szOutput), bitDepthUnknownText.c_str());
					}
					else
					{
						auto bitDepthTemplate =
							m_resourceLoader->LoadString(IDS_GENERAL_DISPLAYWINDOW_BITDEPTH);
						StringCchPrintf(szOutput, std::size(szOutput), bitDepthTemplate.c_str(),
							uBitDepth);
					}

					DisplayWindow_BufferText(m_displayWindow->GetHWND(), szOutput);

					Gdiplus::REAL res;

					res = pimg->GetHorizontalResolution();
					auto horizontalResolutionTemplate = m_resourceLoader->LoadString(
						IDS_GENERAL_DISPLAYWINDOW_HORIZONTALRESOLUTION);
					StringCchPrintf(szOutput, std::size(szOutput),
						horizontalResolutionTemplate.c_str(), res);
					DisplayWindow_BufferText(m_displayWindow->GetHWND(), szOutput);

					res = pimg->GetVerticalResolution();
					auto verticalResolutionTemplate =
						m_resourceLoader->LoadString(IDS_GENERAL_DISPLAYWINDOW_VERTICALRESOLUTION);
					StringCchPrintf(szOutput, std::size(szOutput),
						verticalResolutionTemplate.c_str(), res);
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
				ULARGE_INTEGER ulTotalNumberOfBytes;
				ULARGE_INTEGER ulTotalNumberOfFreeBytes;
				BOOL bRet = GetDiskFreeSpaceEx(fullItemName.c_str(), nullptr, &ulTotalNumberOfBytes,
					&ulTotalNumberOfFreeBytes);

				if (bRet)
				{
					auto sizeText = FormatSizeString(ulTotalNumberOfFreeBytes.QuadPart);
					auto freeSpaceTemplate =
						m_resourceLoader->LoadString(IDS_GENERAL_DISPLAY_WINDOW_FREE_SPACE);
					StringCchPrintf(szMsg, std::size(szMsg), freeSpaceTemplate.c_str(),
						sizeText.c_str());
					DisplayWindow_BufferText(m_displayWindow->GetHWND(), szMsg);

					sizeText = FormatSizeString(ulTotalNumberOfBytes.QuadPart);
					auto totalSizeTemplate =
						m_resourceLoader->LoadString(IDS_GENERAL_DISPLAY_WINDOW_TOTAL_SIZE);
					StringCchPrintf(szMsg, std::size(szMsg), totalSizeTemplate.c_str(),
						sizeText.c_str());
					DisplayWindow_BufferText(m_displayWindow->GetHWND(), szMsg);
				}

				TCHAR szFileSystem[MAX_PATH + 1];
				bRet = GetVolumeInformation(fullItemName.c_str(), nullptr, 0, nullptr, nullptr,
					nullptr, szFileSystem, std::size(szFileSystem));

				if (bRet)
				{
					auto fileSystemTemplate =
						m_resourceLoader->LoadString(IDS_GENERAL_DISPLAY_WINDOW_FILE_SYSTEM);
					StringCchPrintf(szMsg, std::size(szMsg), fileSystemTemplate.c_str(),
						szFileSystem);
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
	int nSelected;

	DisplayWindow_SetThumbnailFile(m_displayWindow->GetHWND(), L"", FALSE);

	nSelected = tab.GetShellBrowserImpl()->GetNumSelected();

	auto multipleItemsText = m_resourceLoader->LoadString(IDS_GENERAL_SELECTED_MULTIPLE_ITEMS);
	StringCchPrintf(szNumSelected, std::size(szNumSelected), _T("%d %s"), nSelected,
		multipleItemsText.c_str());

	DisplayWindow_BufferText(m_displayWindow->GetHWND(), szNumSelected);

	if (!tab.GetShellBrowserImpl()->InVirtualFolder())
	{
		uint64_t selectionSize = tab.GetShellBrowserImpl()->GetSelectionSize();
		auto displayFormat = m_config->globalFolderSettings.forceSize
			? m_config->globalFolderSettings.sizeDisplayFormat
			: +SizeDisplayFormat::None;
		auto selectionSizeText = FormatSizeString(selectionSize, displayFormat);

		auto sizeText = m_resourceLoader->LoadString(IDS_GENERAL_TOTALFILESIZE);
		StringCchPrintf(szTotalSize, std::size(szTotalSize), _T("%s: %s"), sizeText.c_str(),
			selectionSizeText.c_str());
	}

	DisplayWindow_BufferText(m_displayWindow->GetHWND(), szTotalSize);
}

void Explorerplusplus::FolderSizeCallbackStub(int nFolders, int nFiles,
	PULARGE_INTEGER lTotalFolderSize, LPVOID pData)
{
	auto *pfsei = reinterpret_cast<Explorerplusplus::FolderSizeExtraInfo *>(pData);
	reinterpret_cast<Explorerplusplus *>(pfsei->pContainer)
		->FolderSizeCallback(pfsei, nFolders, nFiles, lTotalFolderSize);
	free(pfsei);
}

void Explorerplusplus::FolderSizeCallback(FolderSizeExtraInfo *pfsei, int nFolders, int nFiles,
	PULARGE_INTEGER lTotalFolderSize)
{
	UNREFERENCED_PARAMETER(nFolders);
	UNREFERENCED_PARAMETER(nFiles);

	DWFolderSizeCompletion *pDWFolderSizeCompletion = nullptr;

	pDWFolderSizeCompletion = (DWFolderSizeCompletion *) malloc(sizeof(DWFolderSizeCompletion));

	pDWFolderSizeCompletion->liFolderSize = *lTotalFolderSize;
	pDWFolderSizeCompletion->uId = pfsei->uId;

	/* Queue the result back to the main thread, so that
	the folder size can be displayed. It is up to the main
	thread to determine whether the folder size should actually
	be shown. */
	PostMessage(m_hwnd, WM_APP_FOLDERSIZECOMPLETED, (WPARAM) pDWFolderSizeCompletion, 0);
}
