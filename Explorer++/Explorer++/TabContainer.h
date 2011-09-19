#ifndef TABCONTAINER_INCLUDED
#define TABCONTAINER_INCLUDED

#include "Explorer++_internal.h"
#include "../ShellBrowser/iShellView.h"

class CTabContainer
{
public:

	CTabContainer(HWND hTabCtrl,IShellBrowser2 **pShellBrowsers,IExplorerplusplus *pexpp);
	~CTabContainer();

	void			InsertTab();
	void			RemoveTab();

	int				GetSelection();
	void			SetSelection(int Index);

	IShellBrowser2	*GetBrowserForTab(int Index);

private:

	HWND				m_hTabCtrl;

	IShellBrowser2		**m_pShellBrowsers;
	IExplorerplusplus	*m_pexpp;
};

#endif