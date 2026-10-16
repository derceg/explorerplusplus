// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DrivesToolbar.h"
#include "BrowserWindow.h"
#include "Config.h"
#include "DriveModel.h"
#include "DrivesToolbarView.h"
#include "NavigationHelper.h"
#include "PopupMenuView.h"
#include "ResourceHelper.h"
#include "ResourceLoader.h"
#include "ShellItemContextMenu.h"
#include "../Helper/ShellHelper.h"
#include <ShlObj.h>

class DrivesToolbarButton : public ToolbarButton
{
public:
	DrivesToolbarButton(const std::wstring &path, MouseEventCallback clickedCallback) :
		ToolbarButton(clickedCallback),
		m_path(path)
	{
	}

	std::wstring GetText() const override
	{
		return m_path.substr(0, 1);
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

DrivesToolbar *DrivesToolbar::Create(DrivesToolbarView *view, DriveModel *driveModel,
	BrowserWindow *browser, const AcceleratorManager *acceleratorManager,
	const ResourceLoader *resourceLoader)
{
	return new DrivesToolbar(view, driveModel, browser, acceleratorManager, resourceLoader);
}

DrivesToolbar::DrivesToolbar(DrivesToolbarView *view, DriveModel *driveModel,
	BrowserWindow *browser, const AcceleratorManager *acceleratorManager,
	const ResourceLoader *resourceLoader) :
	ToolbarBase(view, browser, acceleratorManager),
	m_driveModel(driveModel),
	m_browser(browser),
	m_resourceLoader(resourceLoader)
{
	Initialize();
}

DrivesToolbar::~DrivesToolbar() = default;

void DrivesToolbar::Initialize()
{
	AddDrives();

	m_connections.push_back(
		m_driveModel->AddDriveAddedObserver(std::bind_front(&DrivesToolbar::OnDriveAdded, this)));
	m_connections.push_back(m_driveModel->AddDriveUpdatedObserver(
		std::bind_front(&DrivesToolbar::OnDriveUpdated, this)));
	m_connections.push_back(m_driveModel->AddDriveRemovedObserver(
		std::bind_front(&DrivesToolbar::OnDriveRemoved, this)));

	m_view->AddWindowDestroyedObserver(std::bind_front(&DrivesToolbar::OnWindowDestroyed, this));
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

	m_browser->OpenItem(drivePath, DetermineOpenDisposition(false, event.ctrlKey, event.shiftKey));
}

void DrivesToolbar::OnButtonMiddleClicked(const std::wstring &drivePath, const MouseEvent &event)
{
	m_browser->OpenItem(drivePath, DetermineOpenDisposition(true, event.ctrlKey, event.shiftKey));
}

void DrivesToolbar::OnButtonRightClicked(const std::wstring &drivePath, const MouseEvent &event)
{
	ShowContextMenu(drivePath, event.ptScreen, event.shiftKey);
}

void DrivesToolbar::ShowContextMenu(const std::wstring &drivePath, const POINT &ptScreen,
	bool showExtended)
{
	PidlAbsolute fullPidl;
	HRESULT hr = SHParseDisplayName(drivePath.c_str(), nullptr, PidlOutParam(fullPidl), 0, nullptr);

	if (FAILED(hr))
	{
		return;
	}

	auto child = fullPidl.GetLastItem();

	auto parent = fullPidl;
	auto res = parent.RemoveLastItem();
	DCHECK(res);

	ShellItemContextMenu::Flags flags = ShellItemContextMenu::Flags::None;

	if (showExtended)
	{
		WI_SetFlag(flags, ShellItemContextMenu::Flags::ExtendedVerbs);
	}

	PopupMenuView popupMenu(m_view->GetHWND(), m_browser);
	ShellItemContextMenu menu(&popupMenu, m_acceleratorManager, parent, { child },
		m_view->GetHWND(), nullptr, flags, m_browser, m_resourceLoader);
	popupMenu.Show(ptScreen);
}

void DrivesToolbar::OnWindowDestroyed()
{
	delete this;
}
