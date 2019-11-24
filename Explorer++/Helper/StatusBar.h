// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <list>
#include <string>
#include "Macros.h"

class CStatusBar
{
public:

	CStatusBar(HWND hwnd);

	void			SetPartText(int iPart, const TCHAR *szText);

	void			HandleStatusBarMenuOpen(void);
	void			HandleStatusBarMenuClose(void);

private:

	DISALLOW_COPY_AND_ASSIGN(CStatusBar);

	const HWND		m_hwnd;

	int				m_nParts;
	int				*m_pPartWidths;
	std::list<std::wstring>	m_TextList;

	bool			m_bAlteredStatusBarParts;
};