#include "stdafx.h"
#include <list>
#include "Misc.h"
#include "Explorer++.h"

typedef struct
{
	int iImage;
	BOOL bDisabled;
} CTInfo_t;

HWND g_hCTDlg;
HIMAGELIST himlToolbar;

BOOL CALLBACK CustomizeToolbarProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static CContainer *pContainer;

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			pContainer = (CContainer *)lParam;
		}
		break;
	}

	return pContainer->CustomizeToolbarProc(hDlg,uMsg,wParam,lParam);
}

BOOL CALLBACK CContainer::CustomizeToolbarProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			OnCustomizeToolbarInit(hDlg);
			break;

		case WM_MEASUREITEM:
			return OnCustomizeToolbarMeasureItem(hDlg,wParam,lParam);
			break;

		case WM_DRAWITEM:
			return OnCustomizeToolbarDrawItem(wParam,lParam);
			break;

		case WM_COMMAND:
			switch(HIWORD(wParam))
			{
				case LBN_SELCHANGE:
					OnCustomizeToolbarSelChange(hDlg,wParam,lParam);
					break;
			}

			switch(LOWORD(wParam))
			{
				case IDC_CT_BUTTON_ADD:
					OnCustomizeToolbarAdd(hDlg);
					break;

				case IDC_CT_BUTTON_REMOVE:
					OnCustomizeToolbarRemove(hDlg);
					break;

				case IDC_CT_BUTTON_MOVEUP:
					OnCustomizeToolbarMoveUp(hDlg);
					break;

				case IDC_CT_BUTTON_MOVEDOWN:
					OnCustomizeToolbarMoveDown(hDlg);
					break;

				case IDCLOSE:
					EndDialog(hDlg,1);
					break;

				case IDRESET:
					OnCustomizeToolbarReset(hDlg);
					break;
			}
			break;

		case WM_CLOSE:
			EndDialog(hDlg,0);
			break;

		case WM_DESTROY:
			OnCustomizeToolbarDestroy();
			break;
	}

	return 0;
}

void CContainer::OnCustomizeToolbarInit(HWND hDlg)
{
	HWND hListBoxAvailable;
	HWND hListBoxCurrent;
	HBITMAP hBitmap;
	list<ToolbarButton_t>::iterator itr;

	g_hCTDlg = hDlg;

	hListBoxAvailable = GetDlgItem(hDlg,IDC_CT_LISTBOX_AVAILABLE);
	hListBoxCurrent = GetDlgItem(hDlg,IDC_CT_LISTBOX_CURRENT);

	himlToolbar = ImageList_Create(16,16,ILC_COLOR32 | ILC_MASK,0,48);

	hBitmap = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_SHELLIMAGES));

	ImageList_Add(himlToolbar,hBitmap,NULL);

	DeleteObject(hBitmap);

	CustomizeToolbarInsertItems(hDlg);

	/* Select the first item in the available listbox. */
	SendMessage(hListBoxAvailable,LB_SETCURSEL,0,0);

	/* Select the last item in the current listbox. */
	SendMessage(hListBoxCurrent,LB_SETCURSEL,m_tbCurrent.size() - 1,0);

	OnCustomizeToolbarSelectionChangeInternal(hListBoxCurrent);

	SetFocus(hListBoxAvailable);
}

BOOL CContainer::OnCustomizeToolbarMeasureItem(HWND hDlg,WPARAM wParam,LPARAM lParam)
{
	MEASUREITEMSTRUCT *pmi	= NULL;
	HWND hListBox;
	HDC hdc;
	HDC hdcTemp;
	TCHAR szItemString[256];
	HFONT hFont;
	DWORD dwStringDimensions;
	SIZE szString;

	pmi = (MEASUREITEMSTRUCT *)lParam;

	hListBox = GetDlgItem(hDlg,IDC_CT_LISTBOX_AVAILABLE);

	hdc = GetDC(hDlg);
	hdcTemp = CreateCompatibleDC(hdc);

	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SelectObject(hdcTemp,hFont);

	SendMessage(hListBox,LB_GETTEXT,pmi->itemID,(LPARAM)szItemString);

	dwStringDimensions = GetTextExtentPoint(hdcTemp,szItemString,
	lstrlen(szItemString),&szString);

	pmi->itemWidth	= szString.cx;
	pmi->itemHeight	= 20;

	/* Not neccessary to delete stock objects (but
	not harmful). */
	DeleteObject(hFont);
	DeleteDC(hdcTemp);
	ReleaseDC(hDlg,hdc);

	return TRUE;
}

BOOL CContainer::OnCustomizeToolbarDrawItem(WPARAM wParam,LPARAM lParam)
{
	DRAWITEMSTRUCT *pdi	= NULL;
	CTInfo_t *pcti				= NULL;
	TCHAR szItemString[256];
	RECT rcText;
	HBRUSH hBrush;
	int TEXT_LEFT = 20;
	int BITMAP_LEFT = 2;
	int BITMAP_TOP = 2;

	pdi = (DRAWITEMSTRUCT *)lParam;

	pcti = (CTInfo_t *)pdi->itemData;

	if(pdi->itemState & ODS_SELECTED)
	{
		if(pdi->itemState & ODS_FOCUS)
		{
			COLORREF BorderColorBase;
			COLORREF BorderColor = RGB(0,0,0);
			RECT rcSelection;

			BorderColorBase = GetSysColor(COLOR_HIGHLIGHT);

			hBrush = CreateSolidBrush(BorderColor);

			/* Draw the frame around the menu. */
			FrameRect(pdi->hDC,&pdi->rcItem,hBrush);

			DeleteObject(hBrush);

			rcSelection = pdi->rcItem;

			/* Decrease the width and height of the shaded
			rectangle so that the border will be seen
			around the shading. */
			InflateRect(&rcSelection,-1,-1);

			hBrush = CreateSolidBrush(BorderColorBase);

			/* Draw the shading rectangle. */
			FillRect(pdi->hDC,&rcSelection,hBrush);

			DeleteObject(hBrush);
		}
		else
		{
			hBrush = CreateSolidBrush(RGB(255,255,255));

			/* Fill in the default menu backing colour. */
			FillRect(pdi->hDC,&pdi->rcItem,hBrush);

			DeleteObject(hBrush);

			hBrush = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));

			/* Draw the frame around the menu. */
			FrameRect(pdi->hDC,&pdi->rcItem,hBrush);

			DeleteObject(hBrush);
		}
	}
	else if(!(pdi->itemState & ODS_SELECTED))
	{
		hBrush = CreateSolidBrush(RGB(255,255,255));

		/* Fill in the default menu backing colour. */
		FillRect(pdi->hDC,&pdi->rcItem,hBrush);

		DeleteObject(hBrush);
	}

	if(pcti->iImage != -1)
	{
		ImageList_Draw(himlToolbar,pcti->iImage,pdi->hDC,pdi->rcItem.left + BITMAP_LEFT,
			pdi->rcItem.top + BITMAP_TOP,ILD_NORMAL);
	}

	SetBkMode(pdi->hDC,TRANSPARENT);

	if(pcti->bDisabled)
		SetTextColor(pdi->hDC,GetSysColor(COLOR_GRAYTEXT));

	rcText = pdi->rcItem;
	rcText.left += TEXT_LEFT;

	SendMessage(pdi->hwndItem,LB_GETTEXT,pdi->itemID,(LPARAM)szItemString);

	DrawText(pdi->hDC,szItemString,lstrlen(szItemString),&rcText,
		DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_EXPANDTABS | DT_HIDEPREFIX);

	return TRUE;
}

void CContainer::OnCustomizeToolbarDestroy(void)
{
	ImageList_Destroy(himlToolbar);
}

void CContainer::OnCustomizeToolbarAdd(HWND hDlg)
{
	HWND hListBoxAvailable;
	HWND hListBoxCurrent;
	list<ToolbarButton_t>::iterator itrAvailable;
	list<ToolbarButton_t>::iterator itrCurrent;
	CTInfo_t *pcti;
	int iSelectedAvailable;
	int iSelectedCurrent;
	int iNewSelection;
	int nAvailableItems;
	int iIndex;
	int i = 0;

	hListBoxAvailable = GetDlgItem(hDlg,IDC_CT_LISTBOX_AVAILABLE);
	hListBoxCurrent = GetDlgItem(hDlg,IDC_CT_LISTBOX_CURRENT);

	iSelectedAvailable = SendMessage(hListBoxAvailable,LB_GETCURSEL,0,0);
	iSelectedCurrent = SendMessage(hListBoxCurrent,LB_GETCURSEL,0,0);

	itrAvailable = m_tbAvailable.begin();

	for(i = 0;i < iSelectedAvailable;i++)
		itrAvailable++;

	pcti = (CTInfo_t *)SendMessage(hListBoxAvailable,LB_GETITEMDATA,iSelectedAvailable,0);

	/* Don't remove the item if it is the
	first item (a separator) in the available
	listbox. */
	if(iSelectedAvailable != 0)
	{
		/* Remove the item from the available listbox. */
		SendMessage(hListBoxAvailable,LB_DELETESTRING,iSelectedAvailable,0);
	}

	/* Fix up the selection in the available listbox. */
	nAvailableItems = SendMessage(hListBoxAvailable,LB_GETCOUNT,0,0);
	iNewSelection = iSelectedAvailable < (nAvailableItems - 1) ? iSelectedAvailable : (nAvailableItems - 1);
	SendMessage(hListBoxAvailable,LB_SETCURSEL,iNewSelection,0);

	/* Insert the item into the current listbox. */
	iIndex = SendMessage(hListBoxCurrent,LB_INSERTSTRING,iSelectedCurrent,
		(LPARAM)LookupToolbarButtonText(itrAvailable->iItemID));

	SendMessage(hListBoxCurrent,LB_SETITEMDATA,iIndex,(LPARAM)pcti);

	itrCurrent = m_tbCurrent.begin();

	for(i = 0;i < iSelectedCurrent;i++)
		itrCurrent++;

	m_tbCurrent.insert(itrCurrent,*itrAvailable);

	InsertToolbarButton(&(*itrAvailable),iSelectedCurrent);

	if(iSelectedAvailable != 0)
	{
		m_tbAvailable.erase(itrAvailable);
	}
}

void CContainer::OnCustomizeToolbarRemove(HWND hDlg)
{
	HWND hListBoxAvailable;
	HWND hListBoxCurrent;
	list<ToolbarButton_t>::iterator itrAvailable;
	list<ToolbarButton_t>::iterator itrCurrent;
	CTInfo_t *pcti;
	int iSelectedCurrent;
	int iNewSelection;
	int nCurrentItems;
	int iInsertPos;
	int iIndex;
	int i = 0;

	hListBoxAvailable = GetDlgItem(hDlg,IDC_CT_LISTBOX_AVAILABLE);
	hListBoxCurrent = GetDlgItem(hDlg,IDC_CT_LISTBOX_CURRENT);

	iSelectedCurrent = SendMessage(hListBoxCurrent,LB_GETCURSEL,0,0);

	itrCurrent = m_tbCurrent.begin();

	for(i = 0;i < iSelectedCurrent;i++)
		itrCurrent++;

	pcti = (CTInfo_t *)SendMessage(hListBoxCurrent,LB_GETITEMDATA,iSelectedCurrent,0);

	nCurrentItems = SendMessage(hListBoxCurrent,LB_GETCOUNT,0,0);

	if(iSelectedCurrent == (nCurrentItems - 1))
	{
		return;
	}

	/* Remove the item from the current listbox. */
	SendMessage(hListBoxCurrent,LB_DELETESTRING,iSelectedCurrent,0);

	/* Fix up the selection in the current listbox. */
	iNewSelection = iSelectedCurrent < (nCurrentItems - 1) ? iSelectedCurrent : (nCurrentItems - 1);
	SendMessage(hListBoxCurrent,LB_SETCURSEL,iNewSelection,0);
	OnCustomizeToolbarSelectionChangeInternal(hListBoxCurrent);

	if(itrCurrent->iItemID != TOOLBAR_SEPARATOR)
	{
		iInsertPos = CustomizeToolbarDetermineItemPosition(itrCurrent->iItemID);

		/* Insert the item into the available listbox. */
		iIndex = SendMessage(hListBoxAvailable,LB_INSERTSTRING,iInsertPos,
			(LPARAM)LookupToolbarButtonText(itrCurrent->iItemID));

		SendMessage(hListBoxAvailable,LB_SETITEMDATA,iIndex,(LPARAM)pcti);

		SendMessage(hListBoxAvailable,LB_SETCURSEL,iIndex,0);

		itrAvailable = m_tbAvailable.begin();

		for(i = 0;i < iInsertPos;i++)
			itrAvailable++;

		m_tbAvailable.insert(itrAvailable,*itrCurrent);
	}

	DeleteToolbarButton(iSelectedCurrent);

	m_tbCurrent.erase(itrCurrent);
}

void CContainer::OnCustomizeToolbarMoveUp(HWND hDlg)
{
	HWND hListBox;
	int iSelected;

	hListBox = GetDlgItem(hDlg,IDC_CT_LISTBOX_CURRENT);

	iSelected = SendMessage(hListBox,LB_GETCURSEL,0,0);

	CustomizeToolbarSwapItems(hListBox,iSelected,TRUE);
}

void CContainer::OnCustomizeToolbarMoveDown(HWND hDlg)
{
	HWND hListBox;
	int iSelected;

	hListBox = GetDlgItem(hDlg,IDC_CT_LISTBOX_CURRENT);

	iSelected = SendMessage(hListBox,LB_GETCURSEL,0,0);

	CustomizeToolbarSwapItems(hListBox,iSelected,FALSE);
}

void CContainer::OnCustomizeToolbarSelChange(HWND hDlg,WPARAM wParam,LPARAM lParam)
{
	HWND hListBox;

	hListBox = (HWND)lParam;

	if(LOWORD(wParam) == IDC_CT_LISTBOX_CURRENT)
	{
		OnCustomizeToolbarSelectionChangeInternal(hListBox);
	}
}

void CContainer::OnCustomizeToolbarReset(HWND hDlg)
{
	HWND hListBoxAvailable;
	HWND hListBoxCurrent;

	hListBoxAvailable = GetDlgItem(hDlg,IDC_CT_LISTBOX_AVAILABLE);
	hListBoxCurrent = GetDlgItem(hDlg,IDC_CT_LISTBOX_CURRENT);

	/* Set the toolbar buttons back to default. */
	InitializeToolbarButtons();

	/* Reinsert the items into the listboxes. */
	CustomizeToolbarInsertItems(hDlg);

	/* Select the first item in the available listbox. */
	SendMessage(hListBoxAvailable,LB_SETCURSEL,0,0);

	/* Select the first item in the current listbox. */
	SendMessage(hListBoxCurrent,LB_SETCURSEL,0,0);

	OnCustomizeToolbarSelectionChangeInternal(hListBoxCurrent);
}

void CContainer::CustomizeToolbarInsertItems(HWND hDlg)
{
	HWND hListBoxAvailable;
	HWND hListBoxCurrent;
	list<ToolbarButton_t>::iterator itr;
	CTInfo_t *pcti;
	int iIndex;
	int nItems;
	int i = 0;

	hListBoxAvailable = GetDlgItem(hDlg,IDC_CT_LISTBOX_AVAILABLE);
	hListBoxCurrent = GetDlgItem(hDlg,IDC_CT_LISTBOX_CURRENT);

	nItems = SendMessage(hListBoxAvailable,LB_GETCOUNT,0,0);

	for(i = nItems - 1;i >= 0;i--)
	{
		SendMessage(hListBoxAvailable,LB_DELETESTRING,i,0);
	}

	nItems = SendMessage(hListBoxCurrent,LB_GETCOUNT,0,0);

	for(i = nItems - 1;i >= 0;i--)
	{
		SendMessage(hListBoxCurrent,LB_DELETESTRING,i,0);
	}

	for(itr = m_tbAvailable.begin();itr != m_tbAvailable.end();itr++)
	{
		iIndex = SendMessage(hListBoxAvailable,LB_ADDSTRING,0,(LPARAM)LookupToolbarButtonText(itr->iItemID));

		pcti = (CTInfo_t *)malloc(sizeof(CTInfo_t));

		pcti->iImage = LookupToolbarButtonImage(itr->iItemID);
		pcti->bDisabled	= FALSE;

		SendMessage(hListBoxAvailable,LB_SETITEMDATA,iIndex,(LPARAM)pcti);
	}

	for(itr = m_tbCurrent.begin();itr != m_tbCurrent.end();itr++)
	{
		iIndex = SendMessage(hListBoxCurrent,LB_ADDSTRING,0,(LPARAM)LookupToolbarButtonText(itr->iItemID));

		pcti = (CTInfo_t *)malloc(sizeof(CTInfo_t));

		pcti->iImage = LookupToolbarButtonImage(itr->iItemID);
		pcti->bDisabled	= FALSE;

		SendMessage(hListBoxCurrent,LB_SETITEMDATA,iIndex,(LPARAM)pcti);
	}

	pcti->bDisabled = TRUE;
}

void CContainer::CustomizeToolbarSwapItems(HWND hListBox,int iSelected,BOOL bUp)
{
	list<ToolbarButton_t>::iterator itrSelected;
	list<ToolbarButton_t>::iterator itrSwap;
	ToolbarButton_t tb;
	CTInfo_t *pcti;
	TCHAR szText[256];
	int iIndex;
	int iSwap;
	int nItems;
	int i = 0;

	if(bUp)
		iSwap = iSelected -1;
	else
		iSwap = iSelected + 1;

	nItems = SendMessage(hListBox,LB_GETCOUNT,0,0);

	if(iSwap < 0 || iSwap >= nItems)
		return;

	itrSelected = m_tbCurrent.begin();

	for(i = 0;i < iSelected;i++)
		itrSelected++;

	itrSwap = m_tbCurrent.begin();

	for(i = 0;i < iSwap;i++)
		itrSwap++;

	if(bUp)
	{
		tb = *itrSelected;

		m_tbCurrent.erase(itrSelected);

		m_tbCurrent.insert(itrSwap,tb);
	}
	else
	{
		tb = *itrSwap;

		m_tbCurrent.erase(itrSwap);

		m_tbCurrent.insert(itrSelected,tb);
	}

	SendMessage(hListBox,LB_GETTEXT,iSelected,(LPARAM)szText);

	pcti = (CTInfo_t *)SendMessage(hListBox,LB_GETITEMDATA,iSelected,0);

	SendMessage(hListBox,LB_DELETESTRING,iSelected,0);

	iIndex = SendMessage(hListBox,LB_INSERTSTRING,iSwap,(LPARAM)szText);

	SendMessage(hListBox,LB_SETITEMDATA,iIndex,(LPARAM)pcti);

	SendMessage(hListBox,LB_SETCURSEL,iIndex,0);

	OnCustomizeToolbarSelectionChangeInternal(hListBox);
}

void CContainer::OnCustomizeToolbarSelectionChangeInternal(HWND hListBox)
{
	HWND hButtonRemove;
	HWND hButtonMoveUp;
	HWND hButtonMoveDown;
	int iIndex;

	iIndex = SendMessage(hListBox,LB_GETCURSEL,0,0);

	hButtonRemove = GetDlgItem(g_hCTDlg,IDC_CT_BUTTON_REMOVE);
	hButtonMoveUp = GetDlgItem(g_hCTDlg,IDC_CT_BUTTON_MOVEUP);
	hButtonMoveDown = GetDlgItem(g_hCTDlg,IDC_CT_BUTTON_MOVEDOWN);

	if(iIndex == ((int)SendMessage(hListBox,LB_GETCOUNT,0,0) - 1))
	{
		/* The last item in the current listbox
		is selected. Disable all three buttons. */
		EnableWindow(hButtonRemove,FALSE);
		EnableWindow(hButtonMoveUp,FALSE);
		EnableWindow(hButtonMoveDown,FALSE);
	}
	else if(iIndex == 0)
	{
		/* The first item in the current listbox
		is selected. Disable the 'Move Up' button. */
		EnableWindow(hButtonRemove,TRUE);
		EnableWindow(hButtonMoveUp,FALSE);
		EnableWindow(hButtonMoveDown,TRUE);
	}
	else
	{
		EnableWindow(hButtonRemove,TRUE);
		EnableWindow(hButtonMoveUp,TRUE);
		EnableWindow(hButtonMoveDown,TRUE);
	}
}

int CContainer::CustomizeToolbarDetermineItemPosition(int iItemID)
{
	list<ToolbarButton_t>::iterator itr;
	int iPos = 0;
	int i = 0;

	itr = m_tbAvailable.begin();

	for(i = 0;i < sizeof(DefaultToolbarButtonPositions);i++)
	{
		if(itr != m_tbAvailable.end())
		{
			if(itr->iItemID == DefaultToolbarButtonPositions[i])
			{
				iPos++;
				itr++;
			}
		}

		if(DefaultToolbarButtonPositions[i] == iItemID)
			break;
	}

	return iPos;
}