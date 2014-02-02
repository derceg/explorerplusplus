#pragma once

#include <list>
#include <string>
#include "Macros.h"

class CStatusBar
{
public:

	CStatusBar(HWND hwnd);
	~CStatusBar();

	void			SetPartText(int iPart,TCHAR *szText);

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