#ifndef ABOUTDIALOG_INCLUDED
#define ABOUTDIALOG_INCLUDED

#include "../Helper/BaseDialog.h"

class CAboutDialog : public CBaseDialog
{
public:

	CAboutDialog(HINSTANCE hInstance,int iResource,HWND hParent);
	~CAboutDialog();

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnNotify(NMHDR *pnmhdr);
	INT_PTR	OnClose();

private:

	HICON	m_hIcon;
};

#endif