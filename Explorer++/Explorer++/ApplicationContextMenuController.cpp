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
#include "BrowserWindow.h"
#include "MainResource.h"
#include "ResourceLoader.h"

namespace Applications
{

ApplicationContextMenuController::ApplicationContextMenuController(ApplicationModel *model,
	Application *application, ApplicationExecutor *applicationExecutor,
	const BrowserWindow *browser, const ResourceLoader *resourceLoader) :
	m_model(model),
	m_application(application),
	m_applicationExecutor(applicationExecutor),
	m_browser(browser),
	m_resourceLoader(resourceLoader)
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

	auto *editorDialog =
		ApplicationEditorDialog::Create(m_browser->GetHWND(), m_resourceLoader, m_model,
			ApplicationEditorDialog::EditDetails::AddNewApplication(
				std::make_unique<Application>(L"", L""), index));
	editorDialog->ShowModalDialog();
}

void ApplicationContextMenuController::OnDelete()
{
	std::wstring message = m_resourceLoader->LoadString(IDS_APPLICATIONBUTTON_DELETE);
	int messageBoxReturn = MessageBox(m_browser->GetHWND(), message.c_str(), App::APP_NAME,
		MB_YESNO | MB_ICONINFORMATION | MB_DEFBUTTON2);

	if (messageBoxReturn != IDYES)
	{
		return;
	}

	m_model->RemoveItem(m_application);
}

void ApplicationContextMenuController::OnShowProperties()
{
	auto *editorDialog = ApplicationEditorDialog::Create(m_browser->GetHWND(), m_resourceLoader,
		m_model, ApplicationEditorDialog::EditDetails::EditApplication(m_application));
	editorDialog->ShowModalDialog();
}

}
