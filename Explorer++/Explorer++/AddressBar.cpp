// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AddressBar.h"
#include "CoreInterface.h"
#include "DarkModeHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "Tab.h"
#include "TabContainer.h"
#include "../Helper/Controls.h"
#include "../Helper/DataExchangeHelper.h"
#include "../Helper/DataObjectImpl.h"
#include "../Helper/DragDropHelper.h"
#include "../Helper/DropSourceImpl.h"
#include "../Helper/Helper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"
#include <wil/com.h>
#include <wil/common.h>
#include <wil/resource.h>

AddressBar *AddressBar::Create(HWND parent, CoreInterface *coreInterface)
{
	return new AddressBar(parent, coreInterface);
}

AddressBar::AddressBar(HWND parent, CoreInterface *coreInterface) :
	BaseWindow(CreateAddressBar(parent)),
	m_coreInterface(coreInterface),
	m_backgroundBrush(CreateSolidBrush(DARK_MODE_BACKGROUND_COLOR)),
	m_defaultFolderIconIndex(GetDefaultFolderIconIndex())
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

	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (darkModeHelper.IsDarkModeEnabled())
	{
		HWND comboBox = reinterpret_cast<HWND>(SendMessage(m_hwnd, CBEM_GETCOMBOCONTROL, 0, 0));

		darkModeHelper.AllowDarkModeForWindow(comboBox, true);
		SetWindowTheme(comboBox, L"AddressComposited", nullptr);
	}

	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(m_hwnd,
		std::bind_front(&AddressBar::ComboBoxExSubclass, this)));

	HWND hEdit = reinterpret_cast<HWND>(SendMessage(m_hwnd, CBEM_GETEDITCONTROL, 0, 0));
	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(hEdit, EditSubclassStub,
		reinterpret_cast<DWORD_PTR>(this)));

	/* Turn on auto complete for the edit control within the combobox.
	This will let the os complete paths as they are typed. */
	SHAutoComplete(hEdit, SHACF_FILESYSTEM | SHACF_AUTOSUGGEST_FORCE_ON);

	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(parent, ParentWndProcStub,
		reinterpret_cast<DWORD_PTR>(this)));

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
}

LRESULT AddressBar::ComboBoxExSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CTLCOLOREDIT:
		if (auto result = OnComboBoxExCtlColorEdit(reinterpret_cast<HWND>(lParam),
				reinterpret_cast<HDC>(wParam)))
		{
			return *result;
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

std::optional<LRESULT> AddressBar::OnComboBoxExCtlColorEdit(HWND hwnd, HDC hdc)
{
	UNREFERENCED_PARAMETER(hwnd);

	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (!darkModeHelper.IsDarkModeEnabled())
	{
		return std::nullopt;
	}

	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, DarkModeHelper::TEXT_COLOR);

	return reinterpret_cast<LRESULT>(m_backgroundBrush.get());
}

LRESULT CALLBACK AddressBar::EditSubclassStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *addressBar = reinterpret_cast<AddressBar *>(dwRefData);

	return addressBar->EditSubclass(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK AddressBar::EditSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
		m_coreInterface->FocusChanged(WindowFocusSource::AddressBar);
		break;

	case WM_MOUSEWHEEL:
		if (m_coreInterface->OnMouseWheel(MousewheelSource::Other, wParam, lParam))
		{
			return 0;
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK AddressBar::ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *addressBar = reinterpret_cast<AddressBar *>(dwRefData);
	return addressBar->ParentWndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK AddressBar::ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

	m_coreInterface->OpenItem(absolutePath->c_str(),
		m_coreInterface->DetermineOpenDisposition(false, IsKeyDown(VK_CONTROL),
			IsKeyDown(VK_SHIFT)));
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

	auto descriptorStgMedium = GenerateShortcutDescriptorStgMedium(pidlDirectory.get());

	if (!descriptorStgMedium)
	{
		return;
	}

	auto contentsStgMedium = GenerateShortcutContentsStgMedium(pidlDirectory.get());

	if (!contentsStgMedium)
	{
		return;
	}

	FORMATETC ftc[2];
	STGMEDIUM stg[2];

	// The IDataObject instance created below will take ownership of the STGMEDIUMs and the data
	// they contain, which is why they're released here.
	ftc[0] = { static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR)), nullptr,
		DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	stg[0] = descriptorStgMedium->release();

	ftc[1] = { static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_FILECONTENTS)), nullptr,
		DVASPECT_CONTENT, 0, TYMED_ISTREAM };
	stg[1] = contentsStgMedium->release();

	auto dataObject =
		winrt::make_self<DataObjectImpl>(ftc, stg, static_cast<int>(SIZEOF_ARRAY(ftc)));

	wil::com_ptr_nothrow<IDragSourceHelper> dragSourceHelper;
	HRESULT hr = CoCreateInstance(CLSID_DragDropHelper, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&dragSourceHelper));

	if (FAILED(hr))
	{
		return;
	}

	auto dropSource = winrt::make_self<DropSourceImpl>();

	POINT pt = { 0, 0 };
	dragSourceHelper->InitializeFromWindow(m_hwnd, &pt, dataObject.get());

	DWORD dwEffect;
	DoDragDrop(dataObject.get(), dropSource.get(), DROPEFFECT_LINK, &dwEffect);
}

std::optional<wil::unique_stg_medium> AddressBar::GenerateShortcutDescriptorStgMedium(
	PCIDLIST_ABSOLUTE pidl)
{
	auto fileGroupDescriptor = GenerateShortcutDescriptor(pidl);

	if (!fileGroupDescriptor)
	{
		return std::nullopt;
	}

	auto global = WriteDataToGlobal(&*fileGroupDescriptor, sizeof(fileGroupDescriptor));

	if (!global)
	{
		return std::nullopt;
	}

	return wil::unique_stg_medium(GetStgMediumForGlobal(global.release()));
}

std::optional<FILEGROUPDESCRIPTOR> AddressBar::GenerateShortcutDescriptor(PCIDLIST_ABSOLUTE pidl)
{
	FILEGROUPDESCRIPTOR fileGroupDescriptor;
	fileGroupDescriptor.cItems = 1;
	fileGroupDescriptor.fgd[0].dwFlags = FD_ATTRIBUTES;
	fileGroupDescriptor.fgd[0].dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

	// The name of the file will be the item name, followed by .lnk.
	std::wstring displayName;
	HRESULT hr = GetDisplayName(pidl, SHGDN_INFOLDER, displayName);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	displayName += L".lnk";

	hr = StringCchCopy(fileGroupDescriptor.fgd[0].cFileName,
		SIZEOF_ARRAY(fileGroupDescriptor.fgd[0].cFileName), displayName.c_str());

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	return fileGroupDescriptor;
}

std::optional<wil::unique_stg_medium> AddressBar::GenerateShortcutContentsStgMedium(
	PCIDLIST_ABSOLUTE pidl)
{
	wil::com_ptr_nothrow<IShellLink> shellLink;
	HRESULT hr =
		CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink));

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	hr = shellLink->SetIDList(pidl);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	wil::com_ptr_nothrow<IPersistStream> pPersistStream;
	hr = shellLink->QueryInterface(IID_PPV_ARGS(&pPersistStream));

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	wil::com_ptr_nothrow<IStream> stream;
	stream.attach(SHCreateMemStream(nullptr, 0));

	if (!stream)
	{
		return std::nullopt;
	}

	hr = pPersistStream->Save(stream.get(), TRUE);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	hr = stream->Seek({ 0, 0 }, STREAM_SEEK_SET, nullptr);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	return wil::unique_stg_medium(GetStgMediumForStream(stream.detach()));
}

void AddressBar::OnTabSelected(const Tab &tab)
{
	UpdateTextAndIcon(tab);
}

void AddressBar::OnNavigationCommitted(const Tab &tab, PCIDLIST_ABSOLUTE pidl, bool addHistoryEntry)
{
	UNREFERENCED_PARAMETER(pidl);
	UNREFERENCED_PARAMETER(addHistoryEntry);

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

	auto cachedFullPath = entry->GetFullPathForDisplay();
	std::optional<std::wstring> text;

	if (cachedFullPath)
	{
		text = cachedFullPath;
	}
	else
	{
		text = GetFolderPathForDisplay(entry->GetPidl().get());

		if (text)
		{
			entry->SetFullPathForDisplay(*text);
		}
	}

	if (!text)
	{
		return;
	}

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

	SendMessage(m_hwnd, CB_RESETCONTENT, 0, 0);

	UpdateTextAndIconInUI(&*text, iconIndex);
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
