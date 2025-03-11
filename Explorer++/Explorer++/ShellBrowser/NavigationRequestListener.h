// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/PidlHelper.h"
#include <vector>

class NavigationRequest;

// Allows the owner of a NavigationRequest to be informed of various navigation-related events.
class NavigationRequestListener
{
public:
	virtual ~NavigationRequestListener() = default;

	virtual void OnEnumerationCompleted(NavigationRequest *request) = 0;
	virtual void OnEnumerationFailed(NavigationRequest *request) = 0;
	virtual void OnEnumerationStopped(NavigationRequest *request) = 0;
	virtual void OnFinished(NavigationRequest *request) = 0;
};
