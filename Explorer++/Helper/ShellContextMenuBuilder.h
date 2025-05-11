// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <ShObjIdl.h>
#include <string>

class ShellContextMenuIdRemapper;

// Allows items to be added to a shell context menu. Also allows existing shell items to be removed.
class ShellContextMenuBuilder
{
public:
	ShellContextMenuBuilder(HMENU menu, IContextMenu *contextMenu,
		ShellContextMenuIdRemapper *idRemapper);

	void AddStringItem(UINT id, const std::wstring &text, UINT item, bool byPosition);
	void EnableItem(UINT id, bool enable);
	void AddSeparator(UINT item, bool byPosition);
	void AddSubMenuItem(const std::wstring &text, wil::unique_hmenu subMenu, UINT item,
		bool byPosition);
	void RemoveShellItem(const std::wstring &targetVerb);

private:
	void RemapMenuIds(HMENU menu);

	const HMENU m_menu;
	IContextMenu *const m_contextMenu;
	ShellContextMenuIdRemapper *const m_idRemapper;
};
