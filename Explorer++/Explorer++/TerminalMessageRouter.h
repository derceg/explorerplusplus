// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "TabContent.h"

class BrowserWindow;

class TerminalMessageRouter
{
public:
	using Result = TabContent::MessageResult;

	static Result Process(const MSG *msg, BrowserWindow *browser);
};
