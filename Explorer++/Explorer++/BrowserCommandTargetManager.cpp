// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserCommandTargetManager.h"

BrowserCommandTarget *BrowserCommandTargetManager::GetCurrentTarget() const
{
	if (!m_currentTarget)
	{
		return &m_defaultTarget;
	}

	return m_currentTarget;
}

void BrowserCommandTargetManager::SetCurrentTarget(BrowserCommandTarget *currentTarget)
{
	DCHECK(currentTarget);

	if (currentTarget == m_currentTarget)
	{
		return;
	}

	m_currentTarget = currentTarget;

	targetChangedSignal.m_signal();
}

void BrowserCommandTargetManager::ClearCurrentTarget()
{
	if (!m_currentTarget)
	{
		return;
	}

	m_currentTarget = nullptr;

	targetChangedSignal.m_signal();
}

bool BrowserCommandTargetManager::DefaultTarget::IsCommandEnabled(int command) const
{
	UNREFERENCED_PARAMETER(command);

	return false;
}

void BrowserCommandTargetManager::DefaultTarget::ExecuteCommand(int command)
{
	UNREFERENCED_PARAMETER(command);
}
