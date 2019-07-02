// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Tab.h"
#include <boost/signals2.hpp>
#include <unordered_map>

typedef boost::signals2::signal<void(const Tab &tab)> TabSelectedSignal;

// Eventually, this will be driven by a dedicated class, rather than the
// Explorerplusplus class.
__interface TabContainerInterface
{
	void			OnTabSelectionChanged(bool broadcastEvent = true);

	boost::signals2::connection	AddTabSelectedObserver(const TabSelectedSignal::slot_type &observer);
};