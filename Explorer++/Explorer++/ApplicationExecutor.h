// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <string>

namespace Applications
{

class Application;

class ApplicationExecutor
{
public:
	virtual ~ApplicationExecutor() = default;

	virtual void Execute(const Application *application, std::wstring extraParameters) = 0;

	void Execute(const Application *application)
	{
		Execute(application, L"");
	}
};

}
