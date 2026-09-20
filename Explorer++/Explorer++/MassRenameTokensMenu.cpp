// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MassRenameTokensMenu.h"
#include "MainResource.h"
#include "MenuView.h"
#include "ResourceLoader.h"
#include <format>

MassRenameTokensMenu::MassRenameTokensMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, TokenSelectedCallback tokenSelectedCallback,
	const ResourceLoader *resourceLoader) :
	MenuBase(menuView, acceleratorManager),
	m_tokenSelectedCallback(tokenSelectedCallback),
	m_resourceLoader(resourceLoader)
{
	BuildMenu();
}

void MassRenameTokensMenu::BuildMenu()
{
	UINT idCounter = 1;

	for (auto token : MassRenameToken::_values())
	{
		m_menuView->AppendItem(this, idCounter, BuildTokenMenuText(token));
		m_idToTokenMap.insert({ idCounter, token });
		idCounter++;
	}
}

std::wstring MassRenameTokensMenu::BuildTokenMenuText(MassRenameToken token)
{
	std::optional<UINT> stringId;

	switch (token)
	{
	case MassRenameToken::Filename:
		stringId = IDS_MASS_RENAME_TOKENS_MENU_FILENAME;
		break;

	case MassRenameToken::LowercaseFilename:
		stringId = IDS_MASS_RENAME_TOKENS_MENU_LOWERCASE_FILENAME;
		break;

	case MassRenameToken::UppercaseFilename:
		stringId = IDS_MASS_RENAME_TOKENS_MENU_UPPERCASE_FILENAME;
		break;

	case MassRenameToken::Basename:
		stringId = IDS_MASS_RENAME_TOKENS_MENU_BASENAME;
		break;

	case MassRenameToken::Extension:
		stringId = IDS_MASS_RENAME_TOKENS_MENU_EXTENSION;
		break;

	case MassRenameToken::Counter:
		stringId = IDS_MASS_RENAME_TOKENS_MENU_COUNTER;
		break;
	}

	CHECK(stringId);

	return std::format(L"{} {}", GetMassRenameTokenText(token),
		m_resourceLoader->LoadString(*stringId));
}

void MassRenameTokensMenu::OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	UNREFERENCED_PARAMETER(isCtrlKeyDown);
	UNREFERENCED_PARAMETER(isShiftKeyDown);

	auto itr = m_idToTokenMap.find(id);
	CHECK(itr != m_idToTokenMap.end());
	m_tokenSelectedCallback(itr->second);
}
