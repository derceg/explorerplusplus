// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellBrowserFake.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowser/ShellNavigator.h"
#include "../Helper/ShellHelper.h"

ShellBrowserFake::ShellBrowserFake(ShellNavigationController *navigationController) :
	m_navigationController(navigationController)
{
}

HRESULT ShellBrowserFake::NavigateToFolder(const std::wstring &path)
{
	unique_pidl_absolute pidl(SHSimpleIDListFromPath(path.c_str()));

	if (!pidl)
	{
		return E_FAIL;
	}

	auto navigateParams = NavigateParams::Normal(pidl.get());
	return m_navigationController->Navigate(navigateParams);
}

ShellNavigationController *ShellBrowserFake::GetNavigationController() const
{
	return m_navigationController;
}
