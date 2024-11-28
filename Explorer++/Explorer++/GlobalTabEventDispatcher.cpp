// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "GlobalTabEventDispatcher.h"

boost::signals2::connection GlobalTabEventDispatcher::AddPreRemovalObserver(
	const PreRemovalSignal::slot_type &observer)
{
	return m_preRemovalSignal.connect(observer);
}

void GlobalTabEventDispatcher::NotifyPreRemoval(const Tab &tab, int index)
{
	m_preRemovalSignal(tab, index);
}
