// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "SignalHelper.h"
#include <boost/signals2.hpp>
#include <optional>
#include <string>

using MenuHelpTextRequestSignal =
	boost::signals2::signal<std::optional<std::wstring>(HMENU menu, UINT id),
		FirstSuccessfulRequestCombiner<std::optional<std::wstring>>>;

// Represents a UI component that can ultimately display menu help text.
class MenuHelpTextHost
{
public:
	virtual ~MenuHelpTextHost() = default;

	// Allows an observer to respond to help text requests from the host, in order to provide help
	// text for a particular menu item.
	virtual boost::signals2::connection AddMenuHelpTextRequestObserver(
		const MenuHelpTextRequestSignal::slot_type &observer) = 0;
};
