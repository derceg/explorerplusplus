// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/NavigationRequestDelegate.h"
#include <gmock/gmock.h>

class NavigationRequestDelegateMock : public NavigationRequestDelegate
{
public:
	MOCK_METHOD(void, OnEnumerationCompleted, (NavigationRequest * request), (override));
	MOCK_METHOD(void, OnEnumerationFailed, (NavigationRequest * request), (override));
	MOCK_METHOD(void, OnEnumerationStopped, (NavigationRequest * request), (override));
	MOCK_METHOD(void, OnFinished, (NavigationRequest * request), (override));
};
