// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellIconModel.h"

ShellIconModel::ShellIconModel(ShellIconLoader *shellIconLoader, PCIDLIST_ABSOLUTE pidl,
	ShellIconSize size) :
	m_shellIconLoader(shellIconLoader),
	m_pidl(pidl),
	m_size(size)
{
}

wil::unique_hbitmap ShellIconModel::GetBitmap(UINT dpi, IconUpdateCallback updateCallback) const
{
	// The system image list is automatically scaled to the system DPI, so there is no need to
	// perform manual scaling. Note that the automatic scaling isn't ideal, for at least two
	// reasons:
	//
	// 1. Different monitors can have different DPI levels. So, the scaling could be incorrect for
	//    one of the monitors.
	// 2. The DPI can be changed while the application is running. The system image list, however,
	//    will continue to be scaled at the original DPI.
	//
	// So, this may require more work to handle properly.
	UNREFERENCED_PARAMETER(dpi);

	return m_shellIconLoader->LoadShellIcon(m_pidl.Raw(), m_size, updateCallback);
}
