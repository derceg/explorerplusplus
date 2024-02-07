// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/ShellBrowserInterface.h"

class ShellNavigationController;

class ShellBrowserFake : public ShellBrowserInterface
{
public:
	ShellBrowserFake(ShellNavigationController *navigationController);

	HRESULT NavigateToFolder(const std::wstring &path);
	ShellNavigationController *GetNavigationController() const override;

private:
	ShellNavigationController *m_navigationController = nullptr;
};
