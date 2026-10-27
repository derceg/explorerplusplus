// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "RebarBand.h"
#include "ToolbarViewBase.h"

class AcceleratorManager;
class MenuHelpTextHost;

class ToolbarBase : public RebarBand
{
public:
	ToolbarBase(ToolbarViewBase *view, MenuHelpTextHost *menuHelpTextHost,
		const AcceleratorManager *acceleratorManager);
	virtual ~ToolbarBase() = default;

	// RebarBand
	ToolbarViewBase *GetView() override;
	bool CanShowOverflowMenu() const override;
	void ShowOverflowMenu(const POINT &ptScreen) override;

protected:
	const AcceleratorManager *const m_acceleratorManager;

private:
	ToolbarViewBase *const m_view;
	MenuHelpTextHost *const m_menuHelpTextHost;
};
