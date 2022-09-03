// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ApplicationToolbarHelper.h"
#include "Application.h"
#include "CoreInterface.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include <boost/format.hpp>
#include <comdef.h>

namespace Applications
{

namespace ApplicationToolbarHelper
{

// Takes a command string entered by the user, and splits it up into two components: an application
// path (with any environment strings expanded) and a parameter list.
//
// Two supported styles:
// 1. "[command]" [parameters] (needed if the command contains spaces)
// 2. [command] [parameters]
ApplicationInfo ParseCommandString(const std::wstring &command)
{
	std::wstring trimmedCommand = command;
	boost::trim(trimmedCommand);

	if (trimmedCommand.empty())
	{
		return {};
	}

	size_t applicationStart;
	size_t applicationEnd;
	size_t applicationLength;

	if (trimmedCommand[0] == _T('\"'))
	{
		applicationStart = 1;
		applicationEnd = trimmedCommand.find('"', 1);
	}
	else
	{
		applicationStart = 0;
		applicationEnd = trimmedCommand.find_first_of(' ');
	}

	if (applicationEnd != std::wstring::npos)
	{
		applicationLength = applicationEnd - applicationStart;
	}
	else
	{
		applicationLength = std::wstring::npos;
	}

	std::wstring trimmedApplication = trimmedCommand.substr(applicationStart, applicationLength);
	boost::trim(trimmedApplication);

	TCHAR expandedApplicationPath[MAX_PATH];
	MyExpandEnvironmentStrings(trimmedApplication.c_str(), expandedApplicationPath,
		SIZEOF_ARRAY(expandedApplicationPath));

	ApplicationInfo applicationInfo;
	applicationInfo.application = expandedApplicationPath;

	if (applicationEnd != std::wstring::npos)
	{
		applicationInfo.parameters = trimmedCommand.substr(applicationEnd + 1);
		boost::trim(applicationInfo.parameters);
	}

	return applicationInfo;
}

void OpenApplication(CoreInterface *coreInterface, HWND errorDialogParent,
	const Application *application, std::wstring extraParameters)
{
	ApplicationInfo applicationInfo = ParseCommandString(application->GetCommand());

	unique_pidl_absolute pidl;
	HRESULT hr = SHParseDisplayName(applicationInfo.application.c_str(), nullptr,
		wil::out_param(pidl), 0, nullptr);

	if (FAILED(hr))
	{
		std::wstring messageTemplate = ResourceHelper::LoadString(
			coreInterface->GetResourceModule(), IDS_APPLICATION_TOOLBAR_OPEN_ERROR);
		_com_error error(hr);
		std::wstring message =
			(boost::wformat(messageTemplate) % applicationInfo.application % error.ErrorMessage())
				.str();
		MessageBox(errorDialogParent, message.c_str(), NExplorerplusplus::APP_NAME,
			MB_ICONWARNING | MB_OK);
		return;
	}

	std::wstring combinedParameters = applicationInfo.parameters;

	if (!extraParameters.empty())
	{
		combinedParameters += L" " + extraParameters;
	}

	coreInterface->OpenFileItem(pidl.get(), combinedParameters.c_str());
}

}

}
