// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AddressBar.h"
#include "CoreInterface.h"
#include "Navigator.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "Tab.h"
#include "TabContainer.h"
#include "../Helper/Controls.h"
#include "../Helper/DataExchangeHelper.h"
#include "../Helper/DataObjectImpl.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/DragDropHelper.h"
#include "../Helper/DropSourceImpl.h"
#include "../Helper/Helper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"
#include <wil/com.h>
#include <wil/common.h>
#include <wil/resource.h>

AddressBar *AddressBar::Create(HWND parent, CoreInterface *coreInterface, Navigator *navigator)
{
	return new AddressBar(parent, coreInterface, navigator);
}

AddressBar::AddressBar(HWND parent, CoreInterface *coreInterface, Navigator *navigator) :
	BaseWindow(CreateAddressBar(parent)),
	m_coreInterface(coreInterface),
	m_navigator(navigator),
	m_defaultFolderIconIndex(GetDefaultFolderIconIndex()),
	m_fontSetter(m_hwnd, coreInterface->GetConfig())
{
	Initialize(parent);
}

HWND AddressBar::CreateAddressBar(HWND parent)
{
	return CreateComboBox(parent,
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_CLIPSIBLINGS
			| WS_CLIPCHILDREN);
}

void AddressBar::Initialize(HWND parent)
{
	HIMAGELIST smallIcons;
	Shell_GetImageLists(nullptr, &smallIcons);
	SendMessage(m_hwnd, CBEM_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(smallIcons));

	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(m_hwnd,
		std::bind_front(&AddressBar::ComboBoxSubclass, this)));

	HWND hEdit = reinterpret_cast<HWND>(SendMessage(m_hwnd, CBEM_GETEDITCONTROL, 0, 0));
	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(hEdit,
		std::bind_front(&AddressBar::EditSubclass, this)));

	/* Turn on auto complete for the edit control within the combobox.
	This will let the os complete paths as they are typed. */
	SHAutoComplete(hEdit, SHACF_FILESYSTEM | SHACF_AUTOSUGGEST_FORCE_ON);

	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(parent,
		std::bind_front(&AddressBar::ParentWndProc, this)));

	m_coreInterface->AddTabsInitializedObserver(
		[this]
		{
			m_connections.push_back(
				m_coreInterface->GetTabContainer()->tabSelectedSignal.AddObserver(
					std::bind_front(&AddressBar::OnTabSelected, this)));
			m_connections.push_back(
				m_coreInterface->GetTabContainer()->tabNavigationCommittedSignal.AddObserver(
					std::bind_front(&AddressBar::OnNavigationCommitted, this)));
		});

	m_fontSetter.fontUpdatedSignal.AddObserver(std::bind(&AddressBar::OnFontOrDpiUpdated, this));
}

LRESULT AddressBar::ComboBoxSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DPICHANGED_AFTERPARENT:
		OnFontOrDpiUpdated();
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT AddressBar::EditSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_RETURN:
			OnEnterPressed();
			return 0;

		case VK_ESCAPE:
			OnEscapePressed();
			return 0;
		}
		break;

	case WM_SETFOCUS:
		m_coreInterface->FocusChanged();
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT AddressBar::ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hwnd)
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case CBEN_DRAGBEGIN:
				OnBeginDrag();
				break;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void AddressBar::OnEnterPressed()
{
	std::wstring path = GetWindowString(m_hwnd);

	const Tab &selectedTab = m_coreInterface->GetTabContainer()->GetSelectedTab();
	std::wstring currentDirectory = selectedTab.GetShellBrowser()->GetDirectory();

	// When entering a path in the address bar in Windows Explorer, environment variables will be
	// expanded. The behavior here is designed to match that.
	// Note that this does result in potential ambiguity. '%' is a valid character in a filename.
	// That means, for example, it's valid to have a file or folder called %windir%. In cases like
	// that, entering the text %windir% would be ambiguous - the path could refer either to the
	// file/folder or environment variable. Explorer treats it as an environment variable, which is
	// also the behavior here.
	// Additionally, it appears that Explorer doesn't normalize "." in paths (though ".." is
	// normalized). For example, entering "c:\windows\.\" results in an error. Whereas here, the
	// path is normalized before navigation, meaning entering "c:\windows\.\" will result in a
	// navigation to "c:\windows". That also means that entering the relative path ".\" works as
	// expected.
	auto absolutePath = TransformUserEnteredPathToAbsolutePathAndNormalize(path, currentDirectory,
		EnvVarsExpansion::Expand);

	if (!absolutePath)
	{
		// TODO: Should possibly display an error here (perhaps in the status bar).
		return;
	}

	/* TODO: Could keep text user has entered and only revert if navigation fails. */
	// Whether a file or folder is being opened, the address bar text should be reverted to the
	// original text. If the item being opened is a folder, the text will be updated once the
	// navigation commits.
	// Note that if the above call to TransformUserEnteredPathToAbsolutePathAndNormalize() fails,
	// the text won't be reverted. That gives the user the chance to update the text and try again.
	RevertTextInUI();

	m_navigator->OpenItem(*absolutePath,
		m_navigator->DetermineOpenDisposition(false, IsKeyDown(VK_CONTROL), IsKeyDown(VK_SHIFT)));
	m_coreInterface->FocusActiveTab();
}

void AddressBar::OnEscapePressed()
{
	HWND edit = reinterpret_cast<HWND>(SendMessage(m_hwnd, CBEM_GETEDITCONTROL, 0, 0));

	auto modified = SendMessage(edit, EM_GETMODIFY, 0, 0);

	if (modified)
	{
		RevertTextInUI();

		SendMessage(edit, EM_SETSEL, 0, -1);
	}
	else
	{
		m_coreInterface->FocusActiveTab();
	}
}

void AddressBar::OnBeginDrag()
{
	const Tab &selectedTab = m_coreInterface->GetTabContainer()->GetSelectedTab();
	auto pidlDirectory = selectedTab.GetShellBrowser()->GetDirectoryIdl();

	SFGAOF attributes = SFGAO_CANCOPY | SFGAO_CANMOVE | SFGAO_CANLINK;
	HRESULT hr = GetItemAttributes(pidlDirectory.get(), &attributes);

	if (FAILED(hr)
		|| WI_AreAllFlagsClear(attributes, SFGAO_CANCOPY | SFGAO_CANMOVE | SFGAO_CANLINK))
	{
		// The root desktop folder is at least one item that can't be copied/moved/linked to. In a
		// situation like that, it's not possible to start a drag at all.
		return;
	}

	wil::com_ptr_nothrow<IDataObject> dataObject;
	std::vector<PCIDLIST_ABSOLUTE> items = { pidlDirectory.get() };
	hr = CreateDataObjectForShellTransfer(items, &dataObject);

	if (FAILED(hr))
	{
		return;
	}

	DWORD allowedEffects = 0;

	if (WI_IsFlagSet(attributes, SFGAO_CANCOPY))
	{
		WI_SetFlag(allowedEffects, DROPEFFECT_COPY);
	}

	if (WI_IsFlagSet(attributes, SFGAO_CANMOVE))
	{
		WI_SetFlag(allowedEffects, DROPEFFECT_MOVE);
	}

	if (WI_IsFlagSet(attributes, SFGAO_CANLINK))
	{
		WI_SetFlag(allowedEffects, DROPEFFECT_LINK);

		hr = SetPreferredDropEffect(dataObject.get(), DROPEFFECT_LINK);
		assert(SUCCEEDED(hr));
	}

	DWORD effect;
	SHDoDragDrop(m_hwnd, dataObject.get(), nullptr, allowedEffects, &effect);
}

void AddressBar::OnTabSelected(const Tab &tab)
{
	UpdateTextAndIcon(tab);
}

void AddressBar::OnNavigationCommitted(const Tab &tab, const NavigateParams &navigateParams)
{
	UNREFERENCED_PARAMETER(navigateParams);

	if (m_coreInterface->GetTabContainer()->IsTabSelected(tab))
	{
		UpdateTextAndIcon(tab);
	}
}

void AddressBar::UpdateTextAndIcon(const Tab &tab)
{
	// At this point, the text and icon in the address bar are being updated
	// because the current folder has changed (e.g. because another tab has been
	// selected). Therefore, any icon updates for the last history entry can be
	// ignored. If that history entry becomes the current one again (e.g.
	// because the original tab is re-selected), the listener can be set back up
	// (if necessary).
	m_historyEntryUpdatedConnection.disconnect();

	auto entry = tab.GetShellBrowser()->GetNavigationController()->GetCurrentEntry();

	auto fullPathForDisplay = entry->GetFullPathForDisplay();

	auto cachedIconIndex = entry->GetSystemIconIndex();
	int iconIndex;

	if (cachedIconIndex)
	{
		iconIndex = *cachedIconIndex;
	}
	else
	{
		iconIndex = m_defaultFolderIconIndex;

		m_historyEntryUpdatedConnection = entry->historyEntryUpdatedSignal.AddObserver(
			std::bind_front(&AddressBar::OnHistoryEntryUpdated, this));
	}

	UpdateTextAndIconInUI(&fullPathForDisplay, iconIndex);
}

void AddressBar::UpdateTextAndIconInUI(std::wstring *text, int iconIndex)
{
	COMBOBOXEXITEM cbItem;
	cbItem.mask = CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_INDENT;
	cbItem.iItem = -1;
	cbItem.iImage = (iconIndex & 0x0FFF);
	cbItem.iSelectedImage = (iconIndex & 0x0FFF);
	cbItem.iIndent = 1;

	if (text)
	{
		WI_SetFlag(cbItem.mask, CBEIF_TEXT);
		cbItem.pszText = text->data();

		m_currentText = *text;
	}

	SendMessage(m_hwnd, CBEM_SETITEM, 0, reinterpret_cast<LPARAM>(&cbItem));
}

void AddressBar::RevertTextInUI()
{
	SendMessage(m_hwnd, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(m_currentText.c_str()));
}

void AddressBar::OnHistoryEntryUpdated(const HistoryEntry &entry,
	HistoryEntry::PropertyType propertyType)
{
	switch (propertyType)
	{
	case HistoryEntry::PropertyType::SystemIconIndex:
		if (entry.GetSystemIconIndex())
		{
			UpdateTextAndIconInUI(nullptr, *entry.GetSystemIconIndex());
		}
		break;
	}
}

void AddressBar::OnFontOrDpiUpdated()
{
	sizeUpdatedSignal.m_signal();
}
