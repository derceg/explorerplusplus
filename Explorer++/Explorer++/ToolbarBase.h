// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "RebarBand.h"
#include "ToolbarView.h"

class AcceleratorManager;
class MenuHelpTextHost;

class ToolbarBase : public RebarBand
{
public:
	ToolbarBase(ToolbarView *view, MenuHelpTextHost *menuHelpTextHost,
		const AcceleratorManager *acceleratorManager);
	virtual ~ToolbarBase() = default;

	// RebarBand
	ToolbarView *GetView() override;
	bool CanShowOverflowMenu() const override;
	void ShowOverflowMenu(const POINT &ptScreen) override;

protected:
	ToolbarView *const m_view;
	const AcceleratorManager *const m_acceleratorManager;

private:
	MenuHelpTextHost *const m_menuHelpTextHost;
};
