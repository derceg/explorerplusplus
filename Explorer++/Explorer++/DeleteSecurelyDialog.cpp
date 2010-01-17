#include "stdafx.h"
#include <list>
#include "Misc.h"
#include "Explorer++.h"
#include "../Helper/FileOperations.h"

BOOL CALLBACK DeleteSecurelyProcStub(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	static CContainer *pContainer;

	switch(Msg)
	{
		case WM_INITDIALOG:
		{
			pContainer = (CContainer *)lParam;
		}
		break;
	}

	return pContainer->DeleteSecurelyProc(hDlg,Msg,wParam,lParam);
}

BOOL CALLBACK CContainer::DeleteSecurelyProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
		case WM_INITDIALOG:
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
					OnDeleteSecurelyOk(hDlg);
					break;

				case IDCANCEL:
					EndDialog(hDlg,0);
					break;
			}
			break;

		case WM_CLOSE:
			EndDialog(hDlg,0);
			break;
	}

	return 0;
}

void CContainer::OnDeleteSecurelyOk(HWND hDlg)
{
	int res;

	res = MessageBox(hDlg,"You are about to delete this file permanently. "
	"It will not be recoverable.\n Are you sure you want to continue?",
	"Explorer++",MB_ICONWARNING|MB_YESNO);

	if(res == IDYES)
	{
		DeleteFileSecurely("C:\\New Folder (10000)\\Deleted.txt",1);
	}

	EndDialog(hDlg,1);
}