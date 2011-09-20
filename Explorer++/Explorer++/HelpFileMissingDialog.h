#ifndef HELPFILEMISSINGDIALOG_INCLUDED
#define HELPFILEMISSINGDIALOG_INCLUDED

#include "../Helper/BaseDialog.h"

class CHelpFileMissingDialog : public CBaseDialog
{
public:

	CHelpFileMissingDialog(HINSTANCE hInstance,int iResource,HWND hParent);
	~CHelpFileMissingDialog();

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnNotify(NMHDR *pnmhdr);
	INT_PTR	OnClose();
};

#endif