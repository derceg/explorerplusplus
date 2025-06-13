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

wil::unique_hbitmap ShellIconModel::GetBitmap(ShellIconUpdateCallback updateCallback) const
{
	if (!m_pidl.HasValue())
	{
		return nullptr;
	}

	return m_shellIconLoader->LoadShellIcon(m_pidl.Raw(), m_size, updateCallback);
}

bool ShellIconModel::IsEmpty() const
{
	return !m_pidl.HasValue();
}
