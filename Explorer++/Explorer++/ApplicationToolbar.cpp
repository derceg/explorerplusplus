// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ApplicationToolbar.h"
#include "App.h"
#include "Application.h"
#include "ApplicationContextMenu.h"
#include "ApplicationEditorDialog.h"
#include "ApplicationHelper.h"
#include "ApplicationModel.h"
#include "ApplicationToolbarView.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "PopupMenuView.h"
#include "ResourceHelper.h"
#include "../Helper/DragDropHelper.h"
#include "../Helper/MenuHelper.h"
#include <glog/logging.h>

namespace Applications
{

using namespace ApplicationHelper;

class ApplicationToolbarButton : public ToolbarButton
{
public:
	ApplicationToolbarButton(const Application *application, ClickedCallback clickedCallback) :
		ToolbarButton(clickedCallback),
		m_application(application)
	{
	}

	std::wstring GetText() const override
	{
		if (m_application->GetShowNameOnToolbar())
		{
			return m_application->GetName();
		}

		return {};
	}

	std::wstring GetTooltipText() const override
	{
		return std::format(L"{}\n{}", m_application->GetName(), m_application->GetCommand());
	}

	std::optional<int> GetImageIndex() const override
	{
		ApplicationInfo applicationInfo = ParseCommandString(m_application->GetCommand());

		SHFILEINFO shfi;
		DWORD_PTR ret = SHGetFileInfo(applicationInfo.application.c_str(), 0, &shfi, sizeof(shfi),
			SHGFI_SYSICONINDEX);

		// Assign a generic icon if the file wasn't found.
		if (ret == 0)
		{
			return 0;
		}

		return shfi.iIcon;
	}

private:
	const Application *m_application;
};

ApplicationToolbar *ApplicationToolbar::Create(ApplicationToolbarView *view,
	ApplicationModel *model, App *app, CoreInterface *coreInterface, ThemeManager *themeManager)
{
	return new ApplicationToolbar(view, model, app, coreInterface, themeManager);
}

ApplicationToolbar::ApplicationToolbar(ApplicationToolbarView *view, ApplicationModel *model,
	App *app, CoreInterface *coreInterface, ThemeManager *themeManager) :
	m_view(view),
	m_model(model),
	m_applicationExecutor(coreInterface),
	m_app(app),
	m_coreInterface(coreInterface),
	m_themeManager(themeManager)
{
	Initialize();
}

void ApplicationToolbar::Initialize()
{
	AddButtons();

	m_connections.push_back(m_model->AddItemAddedObserver(
		std::bind_front(&ApplicationToolbar::OnApplicationAdded, this)));
	m_connections.push_back(m_model->AddItemUpdatedObserver(
		std::bind_front(&ApplicationToolbar::OnApplicationUpdated, this)));
	m_connections.push_back(m_model->AddItemRemovedObserver(
		std::bind_front(&ApplicationToolbar::OnApplicationRemoved, this)));

	m_connections.push_back(m_coreInterface->AddToolbarContextMenuObserver(
		std::bind_front(&ApplicationToolbar::OnToolbarContextMenuPreShow, this)));
	m_connections.push_back(m_coreInterface->AddToolbarContextMenuSelectedObserver(
		std::bind_front(&ApplicationToolbar::OnToolbarContextMenuItemSelected, this)));

	m_dropTargetWindow = winrt::make_self<DropTargetWindow>(m_view->GetHWND(),
		static_cast<DropTargetInternal *>(this));

	m_view->AddWindowDestroyedObserver(
		std::bind_front(&ApplicationToolbar::OnWindowDestroyed, this));
}

ApplicationToolbarView *ApplicationToolbar::GetView() const
{
	return m_view;
}

void ApplicationToolbar::AddButtons()
{
	size_t index = 0;

	for (auto &application : m_model->GetItems())
	{
		AddButton(application.get(), index);
		++index;
	}
}

void ApplicationToolbar::AddButton(Application *application, size_t index)
{
	auto button = std::make_unique<ApplicationToolbarButton>(application,
		std::bind_front(&ApplicationToolbar::OnButtonClicked, this, application));
	button->SetRightClickedCallback(
		std::bind_front(&ApplicationToolbar::OnButtonRightClicked, this, application));

	m_view->AddButton(std::move(button), index);
}

void ApplicationToolbar::OnApplicationAdded(Application *application, size_t index)
{
	AddButton(application, index);
}

void ApplicationToolbar::OnApplicationUpdated(Application *application)
{
	// The ApplicationToolbarButton class holds a pointer to the associated Application (so there's
	// no need to update it). Only the view needs to be updated.
	auto index = m_model->GetItemIndex(application);
	m_view->UpdateButton(index);
}

void ApplicationToolbar::OnApplicationRemoved(const Application *application, size_t oldIndex)
{
	UNREFERENCED_PARAMETER(application);

	m_view->RemoveButton(oldIndex);
}

void ApplicationToolbar::OnButtonClicked(const Application *application, const MouseEvent &event)
{
	UNREFERENCED_PARAMETER(event);

	m_applicationExecutor.Execute(application);
}

void ApplicationToolbar::OnButtonRightClicked(Application *application, const MouseEvent &event)
{
	POINT ptScreen = event.ptClient;
	ClientToScreen(m_view->GetHWND(), &ptScreen);

	PopupMenuView popupMenu;
	ApplicationContextMenu menu(&popupMenu, m_app->GetAcceleratorManager(), m_model, application,
		&m_applicationExecutor, m_app->GetResourceLoader(), m_coreInterface, m_themeManager);
	popupMenu.Show(m_view->GetHWND(), ptScreen);
}

void ApplicationToolbar::OnToolbarContextMenuPreShow(HMENU menu, HWND sourceWindow, const POINT &pt)
{
	UNREFERENCED_PARAMETER(pt);

	if (sourceWindow != m_view->GetHWND())
	{
		return;
	}

	std::wstring newText = ResourceHelper::LoadString(m_coreInterface->GetResourceInstance(),
		IDS_APPLICATIONBUTTON_NEW);
	MenuHelper::AddStringItem(menu, IDM_APPLICATION_CONTEXT_MENU_NEW, newText,
		IDM_TOOLBARS_CUSTOMIZE, FALSE);
}

void ApplicationToolbar::OnToolbarContextMenuItemSelected(HWND sourceWindow, int menuItemId)
{
	if (sourceWindow != m_view->GetHWND())
	{
		return;
	}

	switch (menuItemId)
	{
	case IDM_APPLICATION_CONTEXT_MENU_NEW:
	{
		ApplicationEditorDialog editorDialog(m_view->GetHWND(),
			m_coreInterface->GetResourceInstance(), m_themeManager, m_model,
			ApplicationEditorDialog::EditDetails::AddNewApplication(
				std::make_unique<Application>(L"", L"")));
		editorDialog.ShowModalDialog();
	}
	break;
	}
}

void ApplicationToolbar::OnWindowDestroyed()
{
	delete this;
}

// DropTargetInternal
DWORD ApplicationToolbar::DragEnter(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(keyState);

	DCHECK(!m_dragData);
	m_dragData = DragData(dataObject,
		std::make_unique<ApplicationDropper>(dataObject, effect, m_model, &m_applicationExecutor));

	return OnDragOver(pt);
}

DWORD ApplicationToolbar::DragOver(DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(effect);

	return OnDragOver(pt);
}

void ApplicationToolbar::DragLeave()
{
	ResetDropState();
}

DWORD ApplicationToolbar::Drop(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(dataObject);
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(effect);

	auto dropLocation = m_view->GetDropLocation(pt);
	auto &dragData = GetDragData();
	DWORD finalEffect =
		dragData.GetApplicationDropper()->PerformDrop(DropLocationToTarget(dropLocation));

	ResetDropState();

	return finalEffect;
}

ApplicationDropper::DropTarget ApplicationToolbar::DropLocationToTarget(
	const ToolbarView::DropLocation &dropLocation)
{
	if (dropLocation.onItem)
	{
		return ApplicationDropper::DropTarget::CreateForDropOnApplication(
			m_model->GetItemAtIndex(dropLocation.index));
	}
	else
	{
		return ApplicationDropper::DropTarget::CreateForDropAtIndex(dropLocation.index);
	}
}

DWORD ApplicationToolbar::OnDragOver(POINT pt)
{
	auto &dragData = GetDragData();

	auto dropLocation = m_view->GetDropLocation(pt);
	auto dropTarget = DropLocationToTarget(dropLocation);
	auto targetEffect = dragData.GetApplicationDropper()->GetDropEffect(dropTarget);

	if (targetEffect != DROPEFFECT_NONE && dropTarget.GetApplication())
	{
		auto applicationInfo =
			ApplicationHelper::ParseCommandString(dropTarget.GetApplication()->GetCommand());

		auto openWithTemplate = ResourceHelper::LoadString(m_coreInterface->GetResourceInstance(),
			IDS_APPLICATION_TOOLBAR_DRAG_OPEN_WITH);
		SetDropDescription(dragData.GetDataObject(), DROPIMAGE_COPY, openWithTemplate,
			applicationInfo.application);
	}
	else
	{
		ClearDropDescription(dragData.GetDataObject());
	}

	return targetEffect;
}

void ApplicationToolbar::ResetDropState()
{
	ClearDropDescription(GetDragData().GetDataObject());

	m_dragData.reset();
}

const ApplicationToolbar::DragData &ApplicationToolbar::GetDragData() const
{
	CHECK(m_dragData);
	return *m_dragData;
}

}
