// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DrivesToolbar.h"
#include "Config.h"
#include "CoreInterface.h"
#include "DrivesToolbarView.h"
#include "Navigation.h"
#include "TabContainer.h"

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
		HRESULT hr = GetItemInfoTip(m_path.c_str(), infoTip);

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

DrivesToolbar *DrivesToolbar::Create(HWND parent, CoreInterface *coreInterface, HINSTANCE instance,
	Navigation *navigation)
{
	return new DrivesToolbar(parent, coreInterface, instance, navigation);
}

DrivesToolbar::DrivesToolbar(HWND parent, CoreInterface *coreInterface, HINSTANCE instance,
	Navigation *navigation) :
	m_view(DrivesToolbarView::Create(parent, coreInterface, instance)),
	m_coreInterface(coreInterface),
	m_navigation(navigation)
{
	Initialize();
}

void DrivesToolbar::Initialize()
{
	AddDrives();

	m_driveModel.AddDriveAddedObserver(std::bind_front(&DrivesToolbar::OnDriveAdded, this));
	m_driveModel.AddDriveUpdatedObserver(std::bind_front(&DrivesToolbar::OnDriveUpdated, this));
	m_driveModel.AddDriveRemovedObserver(std::bind_front(&DrivesToolbar::OnDriveRemoved, this));

	m_view->AddWindowDestroyedObserver(std::bind_front(&DrivesToolbar::OnWindowDestroyed, this));
}

DrivesToolbarView *DrivesToolbar::GetView() const
{
	return m_view;
}

void DrivesToolbar::AddDrives()
{
	const auto &drives = m_driveModel.GetDrives();

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
	auto index = m_driveModel.GetDriveIndex(path);

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

	m_navigation->BrowseFolderInCurrentTab(drivePath.c_str());
}

void DrivesToolbar::OnButtonMiddleClicked(const std::wstring &drivePath, const MouseEvent &event)
{
	bool switchToNewTab = m_coreInterface->GetConfig()->openTabsInForeground;

	if (event.shiftKey)
	{
		switchToNewTab = !switchToNewTab;
	}

	m_coreInterface->GetTabContainer()->CreateNewTab(drivePath.c_str(),
		TabSettings(_selected = switchToNewTab));
}

void DrivesToolbar::OnButtonRightClicked(const std::wstring &drivePath, const MouseEvent &event)
{
	m_view->ShowContextMenu(drivePath, event.ptClient, event.shiftKey);
}

void DrivesToolbar::OnWindowDestroyed()
{
	delete this;
}
