// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ApplicationContextMenuController.h"
#include "App.h"
#include "Application.h"
#include "ApplicationEditorDialog.h"
#include "ApplicationExecutor.h"
#include "ApplicationModel.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "ResourceManager.h"

namespace Applications
{

ApplicationContextMenuController::ApplicationContextMenuController(ApplicationModel *model,
	Application *application, ApplicationExecutor *applicationExecutor,
	CoreInterface *coreInterface) :
	m_model(model),
	m_application(application),
	m_applicationExecutor(applicationExecutor),
	m_coreInterface(coreInterface)
{
}

void ApplicationContextMenuController::OnMenuItemSelected(UINT menuItemId)
{
	switch (menuItemId)
	{
	case IDM_APPLICATION_CONTEXT_MENU_OPEN:
		OnOpen();
		break;

	case IDM_APPLICATION_CONTEXT_MENU_NEW:
		OnNew();
		break;

	case IDM_APPLICATION_CONTEXT_MENU_DELETE:
		OnDelete();
		break;

	case IDM_APPLICATION_CONTEXT_MENU_PROPERTIES:
		OnShowProperties();
		break;

	default:
		DCHECK(false);
		break;
	}
}

void ApplicationContextMenuController::OnOpen()
{
	m_applicationExecutor->Execute(m_application);
}

void ApplicationContextMenuController::OnNew()
{
	auto index = m_model->GetItemIndex(m_application);

	ApplicationEditorDialog editorDialog(m_coreInterface->GetMainWindow(),
		m_coreInterface->GetResourceInstance(), m_model,
		ApplicationEditorDialog::EditDetails::AddNewApplication(
			std::make_unique<Application>(L"", L""), index));
	editorDialog.ShowModalDialog();
}

void ApplicationContextMenuController::OnDelete()
{
	std::wstring message = Resources::LoadString(IDS_APPLICATIONBUTTON_DELETE);
	int messageBoxReturn = MessageBox(m_coreInterface->GetMainWindow(), message.c_str(),
		App::APP_NAME, MB_YESNO | MB_ICONINFORMATION | MB_DEFBUTTON2);

	if (messageBoxReturn != IDYES)
	{
		return;
	}

	m_model->RemoveItem(m_application);
}

void ApplicationContextMenuController::OnShowProperties()
{
	ApplicationEditorDialog editorDialog(m_coreInterface->GetMainWindow(),
		m_coreInterface->GetResourceInstance(), m_model,
		ApplicationEditorDialog::EditDetails::EditApplication(m_application));
	editorDialog.ShowModalDialog();
}

}
