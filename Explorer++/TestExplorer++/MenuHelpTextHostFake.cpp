// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MenuHelpTextHostFake.h"

void MenuHelpTextHostFake::NotifyTopLevelMenuShown()
{
}

void MenuHelpTextHostFake::NotifyTopLevelMenuClosed()
{
}

void MenuHelpTextHostFake::SetMenuHelpText(const std::wstring &text)
{
	m_helpText = text;
}

const std::wstring &MenuHelpTextHostFake::GetCurrentHelpText() const
{
	return m_helpText;
}
