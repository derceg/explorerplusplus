// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class NavigationRequest;

// Allows the owner of a NavigationRequest to be informed of various events. It's expected that in
// response to the enumeration events, the owner will update the state of the navigation.
class NavigationRequestDelegate
{
public:
	virtual ~NavigationRequestDelegate() = default;

	virtual void OnEnumerationCompleted(NavigationRequest *request) = 0;
	virtual void OnEnumerationFailed(NavigationRequest *request) = 0;
	virtual void OnEnumerationStopped(NavigationRequest *request) = 0;

	// Triggered when the navigation has completely finished (i.e. once it transitions into the
	// committed/failed/cancelled state).
	virtual void OnFinished(NavigationRequest *request) = 0;
};
