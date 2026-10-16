// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class AcceleratorManager;
class MenuHelpTextHost;
class ToolbarView;

class ToolbarBase
{
public:
	ToolbarBase(ToolbarView *view, MenuHelpTextHost *menuHelpTextHost,
		const AcceleratorManager *acceleratorManager);
	virtual ~ToolbarBase() = default;

	ToolbarView *GetView() const;
	virtual void ShowOverflowMenu(const POINT &ptScreen);

protected:
	ToolbarView *const m_view;
	const AcceleratorManager *const m_acceleratorManager;

private:
	MenuHelpTextHost *const m_menuHelpTextHost;
};
