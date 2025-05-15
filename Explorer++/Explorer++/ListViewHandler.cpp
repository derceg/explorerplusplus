// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "Config.h"
#include "FolderView.h"
#include "IDropFilesCallback.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "ResourceHelper.h"
#include "ServiceProvider.h"
#include "SetFileAttributesDialog.h"
#include "ShellBrowser/Columns.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowser/ViewModes.h"
#include "SortMenuBuilder.h"
#include "TabContainerImpl.h"
#include "ViewModeHelper.h"
#include "../Helper/BulkClipboardWriter.h"
#include "../Helper/ClipboardHelper.h"
#include "../Helper/DropHandler.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/SystemClipboard.h"
#include "../Helper/WinRTBaseWrapper.h"
#include <wil/com.h>

LRESULT CALLBACK Explorerplusplus::ListViewProcStub(HWND hwnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *pexpp = reinterpret_cast<Explorerplusplus *>(dwRefData);

	return pexpp->ListViewSubclassProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK Explorerplusplus::ListViewSubclassProc(HWND ListView, UINT msg, WPARAM wParam,
	LPARAM lParam)
{
	switch (msg)
	{
	case WM_MENUSELECT:
		SendMessage(m_hContainer, WM_MENUSELECT, wParam, lParam);
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->code)
		{
		case HDN_BEGINDRAG:
			return FALSE;

		case HDN_ENDDRAG:
		{
			/* When the drag ends, the dragged item
			is shifted into position, and all later
			items are shifted down one. Therefore,
			take the item out of its position in the
			list, and move it into its new position. */
			NMHEADER *pnmHeader = nullptr;
			Column_t column;
			int i = 0;

			pnmHeader = (NMHEADER *) lParam;

			auto currentColumns = m_pActiveShellBrowser->GetCurrentColumns();

			i = 0;
			auto itr = currentColumns.begin();
			while (i < (pnmHeader->iItem + 1) && itr != currentColumns.end())
			{
				if (itr->checked)
				{
					i++;
				}

				itr++;
			}

			if (itr != currentColumns.begin())
				itr--;

			column = *itr;
			currentColumns.erase(itr);

			i = 0;
			itr = currentColumns.begin();
			while (i < (pnmHeader->pitem->iOrder + 1) && itr != currentColumns.end())
			{
				if (itr->checked)
				{
					i++;
				}

				itr++;
			}

			if (itr != currentColumns.begin())
				itr--;

			currentColumns.insert(itr, column);

			m_pActiveShellBrowser->SetCurrentColumns(currentColumns);

			Tab &tab = GetActivePane()->GetTabContainerImpl()->GetSelectedTab();
			tab.GetShellBrowserImpl()->GetNavigationController()->Refresh();

			return TRUE;
		}
		}
		break;
	}

	return DefSubclassProc(ListView, msg, wParam, lParam);
}

LRESULT Explorerplusplus::OnListViewKeyDown(LPARAM lParam)
{
	LV_KEYDOWN *keyDown = reinterpret_cast<LV_KEYDOWN *>(lParam);

	switch (keyDown->wVKey)
	{
	case VK_RETURN:
		if (IsKeyDown(VK_MENU))
		{
			m_pActiveShellBrowser->ShowPropertiesForSelectedItems();
		}
		else
		{
			OpenAllSelectedItems(
				DetermineOpenDisposition(false, IsKeyDown(VK_CONTROL), IsKeyDown(VK_SHIFT)));
		}
		break;

	case 'V':
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			OnListViewPaste();
		}
		break;

	case VK_INSERT:
		if (!IsKeyDown(VK_CONTROL) && IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			OnListViewPaste();
		}
		break;
	}

	return 0;
}

void Explorerplusplus::OnListViewClick(const NMITEMACTIVATE *eventInfo)
{
	if (!m_config->globalFolderSettings.oneClickActivate.get())
	{
		return;
	}

	LVHITTESTINFO htInfo = {};
	htInfo.pt = eventInfo->ptAction;
	ListView_HitTest(m_hActiveListView, &htInfo);

	if (WI_IsFlagSet(htInfo.flags, LVHT_ONITEMSTATEICON) && m_config->checkBoxSelection.get())
	{
		// In this case, the click was on the checkbox, so it should be ignored.
		return;
	}

	OnListViewDoubleClick(eventInfo);
}

void Explorerplusplus::OnListViewDoubleClick(const NMITEMACTIVATE *eventInfo)
{
	// Note that while it's stated in the documentation for both NM_CLICK and NM_DBLCLK that "The
	// iItem member of lParam is only valid if the icon or first-column label has been clicked.", it
	// appears that's not actually the case. From testing, iItem will be correctly populated even
	// when the click/double-click takes place elsewhere in a row. Therefore, it should be ok to use
	// that value in this function.
	if (eventInfo->iItem == -1)
	{
		return;
	}

	if (WI_IsFlagSet(eventInfo->uKeyFlags, LVKF_ALT))
	{
		auto pidlDirectory = m_pActiveShellBrowser->GetDirectoryIdl();
		auto pidl = m_pActiveShellBrowser->GetItemChildIdl(eventInfo->iItem);
		ShowMultipleFileProperties(pidlDirectory.get(), { pidl.get() }, m_hContainer);
	}
	else
	{
		OpenListViewItem(eventInfo->iItem,
			DetermineOpenDisposition(false, WI_IsFlagSet(eventInfo->uKeyFlags, LVKF_CONTROL),
				WI_IsFlagSet(eventInfo->uKeyFlags, LVKF_SHIFT)));
	}
}

void Explorerplusplus::OnListViewCopyUniversalPaths() const
{
	if (ListView_GetSelectedCount(m_hActiveListView) == 0)
	{
		return;
	}

	std::wstring strUniversalPaths;
	int iItem = -1;

	while ((iItem = ListView_GetNextItem(m_hActiveListView, iItem, LVNI_SELECTED)) != -1)
	{
		std::wstring fullFilename = m_pActiveShellBrowser->GetItemFullName(iItem);

		TCHAR szBuffer[1024];

		DWORD dwBufferSize = std::size(szBuffer);
		auto *puni = reinterpret_cast<UNIVERSAL_NAME_INFO *>(&szBuffer);
		DWORD dwRet = WNetGetUniversalName(fullFilename.c_str(), UNIVERSAL_NAME_INFO_LEVEL,
			reinterpret_cast<LPVOID>(puni), &dwBufferSize);

		if (dwRet == NO_ERROR)
		{
			strUniversalPaths += puni->lpUniversalName + std::wstring(_T("\r\n"));
		}
		else
		{
			strUniversalPaths += fullFilename + std::wstring(_T("\r\n"));
		}
	}

	strUniversalPaths = strUniversalPaths.substr(0, strUniversalPaths.size() - 2);

	SystemClipboard clipboard;
	BulkClipboardWriter clipboardWriter(&clipboard);
	clipboardWriter.WriteText(strUniversalPaths);
}

void Explorerplusplus::OnListViewSetFileAttributes() const
{
	const Tab &selectedTab = GetActivePane()->GetTabContainerImpl()->GetSelectedTab();
	selectedTab.GetShellBrowserImpl()->SetFileAttributesForSelection();
}

void Explorerplusplus::OnListViewPaste()
{
	wil::com_ptr_nothrow<IDataObject> clipboardObject;
	HRESULT hr = OleGetClipboard(&clipboardObject);

	if (FAILED(hr))
	{
		return;
	}

	const auto &selectedTab = GetActivePane()->GetTabContainerImpl()->GetSelectedTab();
	auto directory = selectedTab.GetShellBrowserImpl()->GetDirectoryIdl();

	if (CanShellPasteDataObject(directory.get(), clipboardObject.get(), PasteType::Normal))
	{
		auto serviceProvider = winrt::make_self<ServiceProvider>();
		serviceProvider->RegisterService(IID_IFolderView,
			winrt::make<FolderView>(selectedTab.GetShellBrowserImpl()->GetWeakPtr()));

		ExecuteActionFromContextMenu(directory.get(), {},
			selectedTab.GetShellBrowserImpl()->GetListView(), L"paste", 0, serviceProvider.get());
	}
	else
	{
		TCHAR szDestination[MAX_PATH + 1];

		/* DO NOT use the internal current directory string.
		 Files are copied asynchronously, so a change of directory
		 will cause the destination directory to change in the
		 middle of the copy operation. */
		StringCchCopy(szDestination, std::size(szDestination),
			selectedTab.GetShellBrowserImpl()->GetDirectoryPath().c_str());

		/* Also, the string must be double NULL terminated. */
		szDestination[lstrlen(szDestination) + 1] = '\0';

		DropHandler *pDropHandler = DropHandler::CreateNew();
		auto *dropFilesCallback = new DropFilesCallback(this);
		pDropHandler->CopyClipboardData(clipboardObject.get(), m_hContainer, szDestination,
			dropFilesCallback);
		pDropHandler->Release();
	}
}

int Explorerplusplus::HighlightSimilarFiles(HWND ListView) const
{
	BOOL bSimilarTypes;
	int iSelected;
	int nItems;
	int nSimilar = 0;
	int i = 0;

	iSelected = ListView_GetNextItem(ListView, -1, LVNI_SELECTED);

	if (iSelected == -1)
		return -1;

	std::wstring testFile = m_pActiveShellBrowser->GetItemFullName(iSelected);

	nItems = ListView_GetItemCount(ListView);

	for (i = 0; i < nItems; i++)
	{
		std::wstring fullFileName = m_pActiveShellBrowser->GetItemFullName(i);

		bSimilarTypes = CompareFileTypes(fullFileName.c_str(), testFile.c_str());

		if (bSimilarTypes)
		{
			ListViewHelper::SelectItem(ListView, i, true);
			nSimilar++;
		}
		else
		{
			ListViewHelper::SelectItem(ListView, i, false);
		}
	}

	return nSimilar;
}

void Explorerplusplus::OpenAllSelectedItems(OpenFolderDisposition openFolderDisposition)
{
	BOOL bSeenDirectory = FALSE;
	DWORD dwAttributes;
	int iItem = -1;
	int iFolderItem = -1;

	while ((iItem = ListView_GetNextItem(m_hActiveListView, iItem, LVIS_SELECTED)) != -1)
	{
		dwAttributes = m_pActiveShellBrowser->GetItemFileFindData(iItem).dwFileAttributes;

		if ((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
		{
			bSeenDirectory = TRUE;
			iFolderItem = iItem;
		}
		else
		{
			OpenListViewItem(iItem);
		}
	}

	if (bSeenDirectory)
	{
		OpenListViewItem(iFolderItem, openFolderDisposition);
	}
}

void Explorerplusplus::OpenListViewItem(int index, OpenFolderDisposition openFolderDisposition)
{
	auto pidlComplete = m_pActiveShellBrowser->GetItemCompleteIdl(index);
	OpenItem(pidlComplete.get(), openFolderDisposition);
}
