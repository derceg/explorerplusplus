// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ScopedBrowserCommandTarget.h"
#include "BrowserCommandTarget.h"
#include "BrowserCommandTargetManager.h"

ScopedBrowserCommandTarget::ScopedBrowserCommandTarget(BrowserCommandTargetManager *targetManager,
	BrowserCommandTarget *target) :
	m_targetManager(targetManager),
	m_target(target)
{
}

void ScopedBrowserCommandTarget::TargetFocused()
{
	m_targetManager->SetCurrentTarget(m_target);
}

ScopedBrowserCommandTarget::~ScopedBrowserCommandTarget()
{
	if (m_targetManager->GetCurrentTarget() == m_target)
	{
		m_targetManager->ClearCurrentTarget();
	}
}
