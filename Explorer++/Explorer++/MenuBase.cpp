// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MenuBase.h"
#include "AcceleratorHelper.h"
#include "AcceleratorManager.h"
#include "MenuView.h"
#include <boost/numeric/conversion/cast.hpp>

MenuBase::MenuBase(MenuView *menuView, const AcceleratorManager *acceleratorManager, UINT startId,
	UINT endId) :
	m_rootMenuView(menuView),
	m_acceleratorManager(acceleratorManager),
	m_idRange(std::max(startId, 1u), std::max({ endId, startId, 1u }))
{
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
