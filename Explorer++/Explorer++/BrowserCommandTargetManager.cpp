// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserCommandTargetManager.h"
#include "BrowserWindow.h"

BrowserCommandTargetManager::BrowserCommandTargetManager(const BrowserWindow *browser) :
	m_browser(browser)
{
}

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

	UpdateCurrentTarget(currentTarget);
}

void BrowserCommandTargetManager::ClearCurrentTarget()
{
	UpdateCurrentTarget(nullptr);
}

void BrowserCommandTargetManager::UpdateCurrentTarget(BrowserCommandTarget *currentTarget)
{
	if (currentTarget == m_currentTarget)
	{
		return;
	}

	m_currentTarget = currentTarget;

	// Broadcasting target changed events when the browser is in anything but its main lifetime
	// state is both unnecessary and unsafe.
	//
	// It's unnecessary during initialization, because the target can change several times and only
	// the last change is relevant. It's unnecessary when the browser is being closed, since the
	// top-level window is going to be destroyed anyway.
	//
	// It's unsafe, because guarantees that are normally upheld during the main lifetime of the
	// browser won't necessarily be upheld while the browser is initializing or closing. For
	// example, assumptions about particular windows existing or there being at least one tab may
	// not hold when the browser is being initialized or when it's being closed.
	if (m_browser->GetLifecycleState() == BrowserWindow::LifecycleState::Main)
	{
		targetChangedSignal.m_signal();
	}
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
