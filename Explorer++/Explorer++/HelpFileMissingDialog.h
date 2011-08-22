#ifndef HELPFILEMISSINGDIALOG_INCLUDED
#define HELPFILEMISSINGDIALOG_INCLUDED

#include "../Helper/BaseDialog.h"

class CHelpFileMissingDialog : public CBaseDialog
{
public:

	CHelpFileMissingDialog(HINSTANCE hInstance,int iResource,HWND hParent);
	~CHelpFileMissingDialog();

protected:

	BOOL	OnInitDialog();
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnNotify(NMHDR *pnmhdr);
	BOOL	OnClose();
};

#endif