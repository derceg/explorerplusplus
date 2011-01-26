#ifndef STATUSBAR_INCLUDED
#define STATUSBAR_INCLUDED

#include <list>
#include <string>

using namespace std;

class CStatusBar
{
public:

	CStatusBar(HWND hwnd);
	~CStatusBar();

	void			SetPartText(int iPart,TCHAR *szText);

	void			HandleStatusBarMenuOpen(void);
	void			HandleStatusBarMenuClose(void);

private:

	HWND			m_hwnd;

	int				m_nParts;
	int				*m_pPartWidths;
	list<wstring>	m_TextList;

	bool			m_bAlteredStatusBarParts;
};

#endif