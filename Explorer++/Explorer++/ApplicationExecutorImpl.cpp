// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ApplicationExecutorImpl.h"
#include "Application.h"
#include "ApplicationHelper.h"
#include "BrowserWindow.h"

namespace Applications
{

ApplicationExecutorImpl::ApplicationExecutorImpl(BrowserWindow *browser) : m_browser(browser)
{
}

void ApplicationExecutorImpl::Execute(const Application *application, std::wstring extraParameters)
{
	auto applicationInfo = ApplicationHelper::ParseCommandString(application->GetCommand());

	std::wstring combinedParameters = applicationInfo.parameters;

	if (!extraParameters.empty())
	{
		combinedParameters += L" " + extraParameters;
	}

	m_browser->OpenFileItem(applicationInfo.application, combinedParameters.c_str());
}

}
