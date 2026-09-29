// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ApplicationContextMenu.h"
#include "AppInfo.h"
#include "ApplicationEditorDialog.h"
#include "ApplicationExecutor.h"
#include "ApplicationModel.h"
#include "BrowserWindow.h"
#include "MainResource.h"
#include "MenuView.h"
#include "ResourceLoader.h"

namespace Applications
{

ApplicationContextMenu::ApplicationContextMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, ApplicationModel *model, Application *application,
	ApplicationExecutor *applicationExecutor, const BrowserWindow *browser,
	const ResourceLoader *resourceLoader) :
	MenuBase(menuView, acceleratorManager),
	m_model(model),
	m_application(application),
	m_applicationExecutor(applicationExecutor),
	m_browser(browser),
	m_resourceLoader(resourceLoader)
{
	BuildMenu();
}

void ApplicationContextMenu::BuildMenu()
{
	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_APPLICATION_CONTEXT_MENU_OPEN))
			.SetHelpText(m_resourceLoader->LoadString(IDS_APPLICATION_CONTEXT_MENU_OPEN_HELP_TEXT))
			.SetSelectedCallback([this] { OnOpen(); }));

	m_rootMenuView->AppendSeparator();

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_APPLICATION_CONTEXT_MENU_NEW))
			.SetHelpText(m_resourceLoader->LoadString(IDS_APPLICATION_CONTEXT_MENU_NEW_HELP_TEXT))
			.SetSelectedCallback([this] { OnNew(); }));

	m_rootMenuView->AppendSeparator();

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_APPLICATION_CONTEXT_MENU_DELETE))
			.SetHelpText(
				m_resourceLoader->LoadString(IDS_APPLICATION_CONTEXT_MENU_DELETE_HELP_TEXT))
			.SetSelectedCallback([this] { OnDelete(); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_APPLICATION_CONTEXT_MENU_PROPERTIES))
			.SetHelpText(
				m_resourceLoader->LoadString(IDS_APPLICATION_CONTEXT_MENU_PROPERTIES_HELP_TEXT))
			.SetSelectedCallback([this] { OnShowProperties(); }));
}

void ApplicationContextMenu::OnOpen()
{
	m_applicationExecutor->Execute(m_application);
}

void ApplicationContextMenu::OnNew()
{
	auto index = m_model->GetItemIndex(m_application);

	auto *editorDialog =
		ApplicationEditorDialog::Create(m_browser->GetHWND(), m_resourceLoader, m_model,
			ApplicationEditorDialog::EditDetails::AddNewApplication(
				std::make_unique<Application>(L"", L""), index));
	editorDialog->ShowModalDialog();
}

void ApplicationContextMenu::OnDelete()
{
	std::wstring message = m_resourceLoader->LoadString(IDS_APPLICATIONBUTTON_DELETE);
	int messageBoxReturn = MessageBox(m_browser->GetHWND(), message.c_str(), AppInfo::NAME,
		MB_YESNO | MB_ICONINFORMATION | MB_DEFBUTTON2);

	if (messageBoxReturn != IDYES)
	{
		return;
	}

	m_model->RemoveItem(m_application);
}

void ApplicationContextMenu::OnShowProperties()
{
	auto *editorDialog = ApplicationEditorDialog::Create(m_browser->GetHWND(), m_resourceLoader,
		m_model, ApplicationEditorDialog::EditDetails::EditApplication(m_application));
	editorDialog->ShowModalDialog();
}

}
