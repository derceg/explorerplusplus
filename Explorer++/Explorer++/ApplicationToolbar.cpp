// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ApplicationToolbar.h"
#include "Application.h"
#include "ApplicationContextMenu.h"
#include "ApplicationExecutor.h"
#include "ApplicationHelper.h"
#include "ApplicationModel.h"
#include "ApplicationToolbarView.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "PopupMenuView.h"
#include "ResourceLoader.h"
#include "TestHelper.h"
#include "../Helper/DragDropHelper.h"
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
	ApplicationModel *model, ApplicationExecutor *applicationExecutor, CoreInterface *coreInterface,
	const AcceleratorManager *acceleratorManager, const ResourceLoader *resourceLoader)
{
	return new ApplicationToolbar(view, model, applicationExecutor, coreInterface,
		acceleratorManager, resourceLoader);
}

ApplicationToolbar::ApplicationToolbar(ApplicationToolbarView *view, ApplicationModel *model,
	ApplicationExecutor *applicationExecutor, CoreInterface *coreInterface,
	const AcceleratorManager *acceleratorManager, const ResourceLoader *resourceLoader) :
	m_view(view),
	m_model(model),
	m_applicationExecutor(applicationExecutor),
	m_coreInterface(coreInterface),
	m_acceleratorManager(acceleratorManager),
	m_resourceLoader(resourceLoader)
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

	m_applicationExecutor->Execute(application);
}

void ApplicationToolbar::OnButtonRightClicked(Application *application, const MouseEvent &event)
{
	POINT ptScreen = event.ptClient;
	ClientToScreen(m_view->GetHWND(), &ptScreen);

	PopupMenuView popupMenu;
	ApplicationContextMenu menu(&popupMenu, m_acceleratorManager, m_model, application,
		m_applicationExecutor, m_resourceLoader, m_coreInterface);
	popupMenu.Show(m_view->GetHWND(), ptScreen);
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
		std::make_unique<ApplicationDropper>(dataObject, effect, m_model, m_applicationExecutor));

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

		auto openWithTemplate =
			m_resourceLoader->LoadString(IDS_APPLICATION_TOOLBAR_DRAG_OPEN_WITH);
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

DWORD ApplicationToolbar::SimulateDropForTest(IDataObject *dataObject, DWORD keyState, POINT pt,
	DWORD effect)
{
	CHECK(IsInTest());

	DragEnter(dataObject, keyState, pt, effect);
	return Drop(dataObject, keyState, pt, effect);
}

}
