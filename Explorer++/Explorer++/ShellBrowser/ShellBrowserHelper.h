// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser.h"

class ShellBrowserHelperBase
{
public:
	ShellBrowserHelperBase(ShellBrowser *shellBrowser);
	virtual ~ShellBrowserHelperBase() = default;

protected:
	ShellBrowser *GetShellBrowser() const;

private:
	ShellBrowser *m_shellBrowser = nullptr;
};

// By inheriting from this class, an object can provide functionality that's tied to an individual
// ShellBrowser instance. For example, a helper could perform an action every time a navigation
// occurs within the associated ShellBrowser.
template <class Derived>
class ShellBrowserHelper : public ShellBrowserHelperBase
{
public:
	template <typename... Args>
	static void CreateAndAttachToShellBrowser(ShellBrowser *shellBrowser, Args &&...args)
	{
		shellBrowser->AddHelper(
			std::make_unique<Derived>(shellBrowser, std::forward<Args>(args)...));
	}

	ShellBrowserHelper(ShellBrowser *shellBrowser) : ShellBrowserHelperBase(shellBrowser)
	{
	}
};
