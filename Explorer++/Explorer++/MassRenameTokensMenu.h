// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MassRenameHelper.h"
#include "MenuBase.h"
#include <functional>
#include <string>

class ResourceLoader;

class MassRenameTokensMenu : public MenuBase
{
public:
	using TokenSelectedCallback = std::function<void(MassRenameToken token)>;

	MassRenameTokensMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		TokenSelectedCallback tokenSelectedCallback, const ResourceLoader *resourceLoader);

private:
	void BuildMenu();
	std::wstring BuildTokenMenuText(MassRenameToken token);

	const TokenSelectedCallback m_tokenSelectedCallback;
	const ResourceLoader *const m_resourceLoader;
};
