// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MenuBase.h"
#include "AcceleratorHelper.h"
#include "AcceleratorManager.h"
#include "MenuView.h"

MenuBase::MenuBase(MenuView *menuView, const AcceleratorManager *acceleratorManager, UINT startId,
	UINT endId) :
	m_menuView(menuView),
	m_acceleratorManager(acceleratorManager),
	m_idRange(std::max(startId, 1u), std::max({ endId, startId, 1u }))
{
	m_connections.push_back(
		menuView->AddViewDestroyedObserver(std::bind_front(&MenuBase::OnViewDestroyed, this)));
}

const MenuBase::IdRange &MenuBase::GetIdRange() const
{
	return m_idRange;
}

std::optional<std::wstring> MenuBase::GetAcceleratorTextForId(UINT id) const
{
	std::optional<ACCEL> accelerator;

	try
	{
		accelerator = m_acceleratorManager->GetAcceleratorForCommand(boost::numeric_cast<WORD>(id));
	}
	catch (const boost::numeric::bad_numeric_cast &)
	{
		DCHECK(false);
	}

	if (!accelerator)
	{
		return std::nullopt;
	}

	return BuildAcceleratorString(*accelerator);
}

void MenuBase::OnViewDestroyed()
{
	// The view should always outlive the associated menu controller, so this method should never be
	// triggered.
	CHECK(false);
}
