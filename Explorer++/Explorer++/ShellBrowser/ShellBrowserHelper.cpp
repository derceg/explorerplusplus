// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowserHelper.h"

ShellBrowserHelperBase::ShellBrowserHelperBase(ShellBrowserInterface *shellBrowser) :
	m_shellBrowser(shellBrowser)
{
}

ShellBrowserInterface *ShellBrowserHelperBase::GetShellBrowser() const
{
	return m_shellBrowser;
}
