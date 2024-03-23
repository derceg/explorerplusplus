// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DrivesToolbar.h"
#include "BrowserWindow.h"
#include "Config.h"
#include "CoreInterface.h"
#include "DriveModel.h"
#include "DrivesToolbarView.h"
#include "MainResource.h"
#include "NavigationHelper.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ShellNavigator.h"
#include "TabContainer.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/ShellHelper.h"
#include <ShlObj.h>
#include <Shlwapi.h>

class DrivesToolbarButton : public ToolbarButton
{
public:
	DrivesToolbarButton(const std::wstring &path, ClickedCallback clickedCallback) :
		ToolbarButton(clickedCallback),
		m_path(path)
	{
	}

	std::wstring GetText() const override
	{
		std::wstring displayText = m_path;

		if (displayText.ends_with(L"\\"))
		{
			displayText.pop_back();
		}

		return displayText;
	}

	std::wstring GetTooltipText() const override
	{
		std::wstring infoTip;
		HRESULT hr = GetItemInfoTip(m_path, infoTip);

		if (FAILED(hr))
		{
			return {};
		}

		return infoTip;
	}

	std::optional<int> GetImageIndex() const override
	{
		SHFILEINFO shfi;
		auto res = SHGetFileInfo(m_path.c_str(), 0, &shfi, sizeof(shfi),
			SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES);

		if (res == 0)
		{
			return std::nullopt;
		}

		return shfi.iIcon;
	}

private:
	std::wstring m_path;
};

DrivesToolbar *DrivesToolbar::Create(DrivesToolbarView *view,
	std::unique_ptr<DriveModel> driveModel, BrowserWindow *browserWindow,
	CoreInterface *coreInterface)
{
	return new DrivesToolbar(view, std::move(driveModel), browserWindow, coreInterface);
}

DrivesToolbar::DrivesToolbar(DrivesToolbarView *view, std::unique_ptr<DriveModel> driveModel,
	BrowserWindow *browserWindow, CoreInterface *coreInterface) :
	m_view(view),
	m_driveModel(std::move(driveModel)),
	m_browserWindow(browserWindow),
	m_coreInterface(coreInterface)
{
	Initialize();
}

DrivesToolbar::~DrivesToolbar() = default;

void DrivesToolbar::Initialize()
{
	AddDrives();

	m_driveModel->AddDriveAddedObserver(std::bind_front(&DrivesToolbar::OnDriveAdded, this));
	m_driveModel->AddDriveUpdatedObserver(std::bind_front(&DrivesToolbar::OnDriveUpdated, this));
	m_driveModel->AddDriveRemovedObserver(std::bind_front(&DrivesToolbar::OnDriveRemoved, this));

	m_view->AddWindowDestroyedObserver(std::bind_front(&DrivesToolbar::OnWindowDestroyed, this));
}

DrivesToolbarView *DrivesToolbar::GetView() const
{
	return m_view;
}

void DrivesToolbar::AddDrives()
{
	const auto &drives = m_driveModel->GetDrives();

	size_t index = 0;

	for (const auto &drive : drives)
	{
		AddDriveAtIndex(drive, index);
		++index;
	}
}

void DrivesToolbar::AddDriveAtIndex(const std::wstring &drivePath, size_t index)
{
	auto button = std::make_unique<DrivesToolbarButton>(drivePath,
		std::bind_front(&DrivesToolbar::OnButtonClicked, this, drivePath));
	button->SetMiddleClickedCallback(
		std::bind_front(&DrivesToolbar::OnButtonMiddleClicked, this, drivePath));
	button->SetRightClickedCallback(
		std::bind_front(&DrivesToolbar::OnButtonRightClicked, this, drivePath));

	m_view->AddButton(std::move(button), index);
}

void DrivesToolbar::OnDriveAdded(const std::wstring &path, size_t index)
{
	AddDriveAtIndex(path, index);
}

void DrivesToolbar::OnDriveUpdated(const std::wstring &path)
{
	auto index = m_driveModel->GetDriveIndex(path);

	if (!index)
	{
		assert(false);
		return;
	}

	m_view->UpdateButton(*index);
}

void DrivesToolbar::OnDriveRemoved(const std::wstring &path, size_t oldIndex)
{
	UNREFERENCED_PARAMETER(path);

	m_view->RemoveButton(oldIndex);
}

void DrivesToolbar::OnButtonClicked(const std::wstring &drivePath, const MouseEvent &event)
{
	UNREFERENCED_PARAMETER(event);

	m_browserWindow->OpenItem(drivePath,
		DetermineOpenDisposition(false, event.ctrlKey, event.shiftKey));
}

void DrivesToolbar::OnButtonMiddleClicked(const std::wstring &drivePath, const MouseEvent &event)
{
	m_browserWindow->OpenItem(drivePath,
		DetermineOpenDisposition(true, event.ctrlKey, event.shiftKey));
}

void DrivesToolbar::OnButtonRightClicked(const std::wstring &drivePath, const MouseEvent &event)
{
	ShowContextMenu(drivePath, event.ptClient, event.shiftKey);
}

void DrivesToolbar::ShowContextMenu(const std::wstring &drivePath, const POINT &ptClient,
	bool showExtended)
{
	unique_pidl_absolute pidl;
	HRESULT hr = SHParseDisplayName(drivePath.c_str(), nullptr, wil::out_param(pidl), 0, nullptr);

	if (FAILED(hr))
	{
		return;
	}

	POINT ptScreen = ptClient;
	ClientToScreen(m_view->GetHWND(), &ptScreen);

	unique_pidl_child child(ILCloneChild(ILFindLastID(pidl.get())));

	[[maybe_unused]] BOOL res = ILRemoveLastID(pidl.get());
	assert(res);

	FileContextMenuManager::Flags flags = FileContextMenuManager::Flags::Standard;

	if (showExtended)
	{
		WI_SetFlag(flags, FileContextMenuManager::Flags::ExtendedVerbs);
	}

	FileContextMenuManager contextMenuManager(m_view->GetHWND(), pidl.get(), { child.get() });
	contextMenuManager.ShowMenu(this, &ptScreen, m_coreInterface->GetStatusBar(), nullptr, flags);
}

void DrivesToolbar::UpdateMenuEntries(HMENU menu, PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, IContextMenu *contextMenu)
{
	UNREFERENCED_PARAMETER(pidlParent);
	UNREFERENCED_PARAMETER(pidlItems);
	UNREFERENCED_PARAMETER(contextMenu);

	std::wstring openInNewTabText = ResourceHelper::LoadString(
		m_coreInterface->GetResourceInstance(), IDS_GENERAL_OPEN_IN_NEW_TAB);
	MenuHelper::AddStringItem(menu, OPEN_IN_NEW_TAB_MENU_ITEM_ID, openInNewTabText, 1, TRUE);
}

bool DrivesToolbar::HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, const std::wstring &verb)
{
	if (verb == L"open")
	{
		assert(pidlItems.size() == 1);

		unique_pidl_absolute pidl(ILCombine(pidlParent, pidlItems[0].Raw()));
		m_browserWindow->OpenItem(pidl.get());
		return true;
	}

	return false;
}

void DrivesToolbar::HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, int cmd)
{
	UNREFERENCED_PARAMETER(pidlItems);

	switch (cmd)
	{
	case OPEN_IN_NEW_TAB_MENU_ITEM_ID:
	{
		assert(pidlItems.size() == 1);

		unique_pidl_absolute pidl(ILCombine(pidlParent, pidlItems[0].Raw()));
		auto navigateParams = NavigateParams::Normal(pidl.get());
		m_coreInterface->GetTabContainer()->CreateNewTab(navigateParams,
			TabSettings(_selected = m_coreInterface->GetConfig()->openTabsInForeground));
	}
	break;
	}
}

void DrivesToolbar::OnWindowDestroyed()
{
	delete this;
}
