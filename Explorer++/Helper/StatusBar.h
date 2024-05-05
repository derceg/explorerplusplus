// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/core/noncopyable.hpp>
#include <list>
#include <string>

class StatusBar : private boost::noncopyable
{
public:
	StatusBar(HWND hwnd);

	void SetPartText(int iPart, const TCHAR *szText);

	void HandleStatusBarMenuOpen();
	void HandleStatusBarMenuClose();

private:
	const HWND m_hwnd;

	int m_nParts;
	int *m_pPartWidths;
	std::list<std::wstring> m_TextList;

	bool m_bAlteredStatusBarParts;
};
