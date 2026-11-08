// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ToolbarViewBase.h"
#include <memory>
#include <vector>

// This represents a toolbar that is fully managed internally (i.e. the toolbar can't be externally
// customized). Unlike CustomizableToolbarView, this class owns the authoritative set of buttons,
// with the control simply mirroring the internal data. That also means that this class provides
// programmatic ways to add or remove buttons.
class ToolbarView : public ToolbarViewBase
{
public:
	ToolbarView(ViewBase *parent, DWORD style, DWORD extendedStyle,
		MenuHelpTextHost *menuHelpTextHost, const Config *config);

	void AddButton(std::unique_ptr<ToolbarButton> button, size_t index);
	ToolbarButton *GetButtonAtIndex(size_t index) const;
	void RemoveButton(const ToolbarButton *button);

private:
	// ToolbarViewBase
	ToolbarButton *GetButtonById(int id) const override;

	int m_idCounter = INITIAL_ID;
	std::vector<ButtonEntry> m_buttons;
};
