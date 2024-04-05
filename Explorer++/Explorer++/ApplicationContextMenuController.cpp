// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ApplicationContextMenuController.h"
#include "Application.h"
#include "ApplicationEditorDialog.h"
#include "ApplicationExecutor.h"
#include "ApplicationHelper.h"
#include "ApplicationModel.h"
#include "CoreInterface.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "ResourceHelper.h"

namespace Applications
{

ApplicationContextMenuController::ApplicationContextMenuController(
	ApplicationExecutor *applicationExecutor, CoreInterface *coreInterface) :
	m_applicationExecutor(applicationExecutor),
	m_coreInterface(coreInterface)
{
}

void ApplicationContextMenuController::OnMenuItemSelected(UINT menuItemId, ApplicationModel *model,
	Application *targetApplication, size_t targetIndex, HWND parentWindow)
{
	switch (menuItemId)
	{
	case IDM_APP_OPEN:
		OnOpen(targetApplication);
		break;

	case IDM_APP_PROPERTIES:
		OnShowProperties(parentWindow, model, targetApplication);
		break;

	case IDM_APP_DELETE:
		OnDelete(model, targetApplication, parentWindow);
		break;

	case IDM_APP_NEW:
		OnNew(parentWindow, model, targetIndex);
		break;
	}
}

void ApplicationContextMenuController::OnOpen(const Application *targetApplication)
{
	m_applicationExecutor->Execute(targetApplication);
}

void ApplicationContextMenuController::OnShowProperties(HWND parentWindow, ApplicationModel *model,
	Application *targetApplication)
{
	ApplicationEditorDialog editorDialog(parentWindow, m_coreInterface->GetResourceInstance(),
		model, ApplicationEditorDialog::EditDetails::EditApplication(targetApplication));
	editorDialog.ShowModalDialog();
}

void ApplicationContextMenuController::OnDelete(ApplicationModel *model,
	const Application *targetApplication, HWND parentWindow)
{
	std::wstring message = ResourceHelper::LoadString(m_coreInterface->GetResourceInstance(),
		IDS_APPLICATIONBUTTON_DELETE);
	int messageBoxReturn = MessageBox(parentWindow, message.c_str(), NExplorerplusplus::APP_NAME,
		MB_YESNO | MB_ICONINFORMATION | MB_DEFBUTTON2);

	if (messageBoxReturn != IDYES)
	{
		return;
	}

	model->RemoveItem(targetApplication);
}

void ApplicationContextMenuController::OnNew(HWND parentWindow, ApplicationModel *model,
	size_t index)
{
	ApplicationEditorDialog editorDialog(parentWindow, m_coreInterface->GetResourceInstance(),
		model,
		ApplicationEditorDialog::EditDetails::AddNewApplication(
			std::make_unique<Application>(L"", L""), index));
	editorDialog.ShowModalDialog();
}

}
