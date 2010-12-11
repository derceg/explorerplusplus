#ifndef ABOUTDIALOG_INCLUDED
#define ABOUTDIALOG_INCLUDED

#include "../Helper/BaseDialog.h"

class CAboutDialog : public CBaseDialog
{
public:

	CAboutDialog(HINSTANCE hInstance,int iResource,HWND hParent);
	~CAboutDialog();

protected:

	BOOL	OnInitDialog();
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnNotify(NMHDR *pnmhdr);
	BOOL	OnClose();

private:

	HICON	m_hIcon;
};

#endif