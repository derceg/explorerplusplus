/******************************************************************
 *
 * Project: Explorer++
 * File: SelectColumnsDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the 'Select Columns' dialog.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"


BOOL	g_bColumnsSwapped = FALSE;
int		g_nChecked = 0;

INT_PTR CALLBACK SelectColumnsProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static CContainer *pContainer = NULL;

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			pContainer = (CContainer *)lParam;
		}
		break;
	}

	return pContainer->SelectColumnsProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK CContainer::SelectColumnsProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
		case WM_INITDIALOG:
			OnInitColumnDlg(hDlg);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_COLUMNS_MOVEUP:
					MoveColumnItem2(hDlg,TRUE);
					break;

				case IDC_COLUMNS_MOVEDOWN:
					MoveColumnItem2(hDlg,FALSE);
					break;

				case IDC_COLUMNS_ENABLE:
					EnableColumnItem(hDlg,TRUE);
					break;

				case IDC_COLUMNS_DISABLE:
					EnableColumnItem(hDlg,FALSE);
					break;

				case IDOK:
					OnColumnDlgOk(hDlg);
					break;

				case IDCANCEL:
					SelectColumnsSaveState(hDlg);
					EndDialog(hDlg,0);
					break;
			}
			break;

		case WM_NOTIFY:
			{
				NMHDR *nmhdr;

				nmhdr = (NMHDR *)lParam;

				switch(nmhdr->code)
				{
					case LVN_ITEMCHANGING:
						return OnColumnDlgLvnItemChanging(hDlg,lParam);
						break;
				}
			}
			break;

		case WM_CLOSE:
			SelectColumnsSaveState(hDlg);
			EndDialog(hDlg,0);
			break;
	}

	return 0;
}

void CContainer::OnInitColumnDlg(HWND hDlg)
{
	HWND						hListView;
	list<Column_t>				pActiveColumnList;
	list<Column_t>::iterator	itr;
	LVITEM						lvItem;
	LVCOLUMN					lvColumn;
	TCHAR						szText[64];
	int							iItem = 0;

	if(m_bSelectColumnsDlgStateSaved)
	{
		SetWindowPos(hDlg,NULL,m_ptSelectColumns.x,
			m_ptSelectColumns.y,0,0,SWP_NOSIZE|SWP_NOZORDER);
	}
	else
	{
		CenterWindow(m_hContainer,hDlg);
	}

	hListView = GetDlgItem(hDlg,IDC_COLUMNS_LISTVIEW);

	ListView_SetExtendedListViewStyleEx(hListView,
	LVS_EX_CHECKBOXES,LVS_EX_CHECKBOXES);

	lvColumn.mask	= LVCF_WIDTH;
	lvColumn.cx		= 180;
	ListView_InsertColumn(hListView,0,&lvColumn);

	m_pActiveShellBrowser->ExportCurrentColumns(&pActiveColumnList);

	/* MUST set this beforehand. If it is 1 while items are
	been inserted, their checkbox may not appear and the
	dialog may appear corrupted. */
	g_nChecked = 0;

	for(itr = pActiveColumnList.begin();itr != pActiveColumnList.end();itr++)
	{
		LoadString(g_hLanguageModule,LookupColumnNameStringIndex(itr->id),
			szText,SIZEOF_ARRAY(szText));

		lvItem.mask		= LVIF_TEXT | LVIF_PARAM;
		lvItem.iItem	= iItem;
		lvItem.iSubItem	= 0;
		lvItem.pszText	= szText;
		lvItem.lParam	= itr->id;

		ListView_InsertItem(hListView,&lvItem);

		ListView_SetCheckState(hListView,iItem,itr->bChecked);

		iItem++;
	}

	g_nChecked = 0;

	for(itr = pActiveColumnList.begin();itr != pActiveColumnList.end();itr++)
	{
		if(itr->bChecked)
			g_nChecked++;
	}

	g_bColumnsSwapped = FALSE;

	ListView_SelectItem(hListView,0,TRUE);
	SetFocus(hListView);
}

void CContainer::MoveColumnItem2(HWND hDlg,BOOL bUp)
{
	HWND hListView;
	int iSelected;

	hListView = GetDlgItem(hDlg,IDC_COLUMNS_LISTVIEW);

	iSelected = ListView_GetNextItem(hListView,-1,LVNI_SELECTED);

	if(iSelected != -1)
	{
		if(bUp)
			ListView_SwapItems(hListView,iSelected,iSelected - 1);
		else
			ListView_SwapItems(hListView,iSelected,iSelected + 1);

		g_bColumnsSwapped = TRUE;

		SetFocus(hListView);
	}
}

void CContainer::EnableColumnItem(HWND hDlg,BOOL bEnable)
{
	HWND hListView;
	int iSelected;

	hListView = GetDlgItem(hDlg,IDC_COLUMNS_LISTVIEW);

	iSelected = ListView_GetNextItem(hListView,-1,LVNI_SELECTED);

	if(iSelected != -1)
	{
		ListView_SetCheckState(hListView,iSelected,bEnable);
	}
}

BOOL CContainer::OnColumnDlgLvnItemChanging(HWND hDlg,LPARAM lParam)
{
	NMLISTVIEW *nmlv;
	HWND hListView;

	nmlv = (NMLISTVIEW *)lParam;

	hListView = GetDlgItem(hDlg,IDC_COLUMNS_LISTVIEW);

	if((nmlv->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(2))
	{
		/* The item was checked. */
		EnableWindow(GetDlgItem(hDlg,IDC_COLUMNS_ENABLE),FALSE);
		EnableWindow(GetDlgItem(hDlg,IDC_COLUMNS_DISABLE),TRUE);

		g_nChecked++;
	}
	else if((nmlv->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(1))
	{
		/* The item was unchecked. */

		if(g_nChecked != 1)
		{
			EnableWindow(GetDlgItem(hDlg,IDC_COLUMNS_DISABLE),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_COLUMNS_ENABLE),TRUE);

			g_nChecked--;
		}
		else
		{
			/* Can't just return. Need to set the DWL_MSGRESULT
			value to the return value first. */
			SetWindowLongPtr(hDlg,DWLP_MSGRESULT,TRUE);
			return TRUE;
		}
	}
	else if(nmlv->uNewState & LVIS_SELECTED)
	{
		LVITEM lvItem;
		TCHAR szColumnDescription[128];
		int iDescriptionStringIndex;

		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= nmlv->iItem;
		lvItem.iSubItem	= 0;

		ListView_GetItem(hListView,&lvItem);

		iDescriptionStringIndex = LookupColumnDescriptionStringIndex((int)lvItem.lParam);

		LoadString(g_hLanguageModule,iDescriptionStringIndex,szColumnDescription,
		SIZEOF_ARRAY(szColumnDescription));
		SetDlgItemText(hDlg,IDC_COLUMNS_DESCRIPTION,szColumnDescription);

		if(ListView_GetCheckState(hListView,nmlv->iItem))
		{
			EnableWindow(GetDlgItem(hDlg,IDC_COLUMNS_ENABLE),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_COLUMNS_DISABLE),TRUE);
		}
		else
		{
			EnableWindow(GetDlgItem(hDlg,IDC_COLUMNS_DISABLE),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_COLUMNS_ENABLE),TRUE);
		}
	}

	return FALSE;
}

void CContainer::OnColumnDlgOk(HWND hDlg)
{
	HWND hListView;
	LVITEM lvItem;
	list<Column_t> ColumnTempList;
	Column_t Column;
	int i = 0;

	hListView = GetDlgItem(hDlg,IDC_COLUMNS_LISTVIEW);

	for(i = 0;i < ListView_GetItemCount(hListView);i++)
	{
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= i;
		lvItem.iSubItem	= 0;
		ListView_GetItem(hListView,&lvItem);

		Column.id		= (int)lvItem.lParam;
		Column.bChecked	= ListView_GetCheckState(hListView,i);
		Column.iWidth	= DEFAULT_COLUMN_WIDTH;
		ColumnTempList.push_back(Column);
	}

	m_pActiveShellBrowser->ImportColumns(&ColumnTempList,g_bColumnsSwapped);

	if(g_bColumnsSwapped)
		RefreshTab(m_iObjectIndex);

	SelectColumnsSaveState(hDlg);
	EndDialog(hDlg,1);
}

void CContainer::SelectColumnsSaveState(HWND hDlg)
{
	RECT rcTemp;

	GetWindowRect(hDlg,&rcTemp);
	m_ptSelectColumns.x = rcTemp.left;
	m_ptSelectColumns.y = rcTemp.top;

	m_bSelectColumnsDlgStateSaved = TRUE;
}

int CContainer::LookupColumnNameStringIndex(int iColumnId)
{
	switch(iColumnId)
	{
	case CM_NAME:
		return IDS_COLUMN_NAME_NAME;
		break;

	case CM_TYPE:
		return IDS_COLUMN_NAME_TYPE;
		break;

	case CM_SIZE:
		return IDS_COLUMN_NAME_SIZE;
		break;

	case CM_DATEMODIFIED:
		return IDS_COLUMN_NAME_DATEMODIFIED;
		break;

	case CM_ATTRIBUTES:
		return IDS_COLUMN_NAME_ATTRIBUTES;
		break;

	case CM_REALSIZE:
		return IDS_COLUMN_NAME_REALSIZE;
		break;

	case CM_SHORTNAME:
		return IDS_COLUMN_NAME_SHORTNAME;
		break;

	case CM_OWNER:
		return IDS_COLUMN_NAME_OWNER;
		break;

	case CM_PRODUCTNAME:
		return IDS_COLUMN_NAME_PRODUCTNAME;
		break;

	case CM_COMPANY:
		return IDS_COLUMN_NAME_COMPANY;
		break;

	case CM_DESCRIPTION:
		return IDS_COLUMN_NAME_DESCRIPTION;
		break;

	case CM_FILEVERSION:
		return IDS_COLUMN_NAME_FILEVERSION;
		break;

	case CM_PRODUCTVERSION:
		return IDS_COLUMN_NAME_PRODUCTVERSION;
		break;

	case CM_SHORTCUTTO:
		return IDS_COLUMN_NAME_SHORTCUTTO;
		break;

	case CM_HARDLINKS:
		return IDS_COLUMN_NAME_HARDLINKS;
		break;

	case CM_EXTENSION:
		return IDS_COLUMN_NAME_EXTENSION;
		break;

	case CM_CREATED:
		return IDS_COLUMN_NAME_CREATED;
		break;

	case CM_ACCESSED:
		return IDS_COLUMN_NAME_ACCESSED;
		break;

	case CM_TITLE:
		return IDS_COLUMN_NAME_TITLE;
		break;

	case CM_SUBJECT:
		return IDS_COLUMN_NAME_SUBJECT;
		break;

	case CM_AUTHOR:
		return IDS_COLUMN_NAME_AUTHOR;
		break;

	case CM_KEYWORDS:
		return IDS_COLUMN_NAME_KEYWORDS;
		break;

	case CM_COMMENT:
		return IDS_COLUMN_NAME_COMMENT;
		break;

	case CM_CAMERAMODEL:
		return IDS_COLUMN_NAME_CAMERAMODEL;
		break;

	case CM_DATETAKEN:
		return IDS_COLUMN_NAME_DATETAKEN;
		break;

	case CM_WIDTH:
		return IDS_COLUMN_NAME_WIDTH;
		break;

	case CM_HEIGHT:
		return IDS_COLUMN_NAME_HEIGHT;
		break;

	case CM_VIRTUALCOMMENTS:
		return IDS_COLUMN_NAME_VIRTUALCOMMENTS;
		break;

	case CM_TOTALSIZE:
		return IDS_COLUMN_NAME_TOTALSIZE;
		break;

	case CM_FREESPACE:
		return IDS_COLUMN_NAME_FREESPACE;
		break;

	case CM_FILESYSTEM:
		return IDS_COLUMN_NAME_FILESYSTEM;
		break;

	case CM_VIRTUALTYPE:
		return IDS_COLUMN_NAME_VIRTUALTYPE;
		break;

	case CM_ORIGINALLOCATION:
		return IDS_COLUMN_NAME_ORIGINALLOCATION;
		break;

	case CM_DATEDELETED:
		return IDS_COLUMN_NAME_DATEDELETED;
		break;

	case CM_NUMPRINTERDOCUMENTS:
		return IDS_COLUMN_NAME_NUMPRINTERDOCUMENTS;
		break;

	case CM_PRINTERSTATUS:
		return IDS_COLUMN_NAME_PRINTERSTATUS;
		break;

	case CM_PRINTERCOMMENTS:
		return IDS_COLUMN_NAME_PRINTERCOMMENTS;
		break;

	case CM_PRINTERLOCATION:
		return IDS_COLUMN_NAME_PRINTERLOCATION;
		break;

	case CM_PRINTERMODEL:
		return IDS_COLUMN_NAME_PRINTERMODEL;
		break;

	case CM_NETWORKADAPTER_STATUS:
		return IDS_COLUMN_NAME_NETWORKADAPTER_STATUS;
		break;

	case CM_MEDIA_BITRATE:
		return IDS_COLUMN_NAME_BITRATE;
		break;

	case CM_MEDIA_COPYRIGHT:
		return IDS_COLUMN_NAME_COPYRIGHT;
		break;

	case CM_MEDIA_DURATION:
		return IDS_COLUMN_NAME_DURATION;
		break;

	case CM_MEDIA_PROTECTED:
		return IDS_COLUMN_NAME_PROTECTED;
		break;

	case CM_MEDIA_RATING:
		return IDS_COLUMN_NAME_RATING;
		break;

	case CM_MEDIA_ALBUMARTIST:
		return IDS_COLUMN_NAME_ALBUMARTIST;
		break;

	case CM_MEDIA_ALBUM:
		return IDS_COLUMN_NAME_ALBUM;
		break;

	case CM_MEDIA_BEATSPERMINUTE:
		return IDS_COLUMN_NAME_BEATSPERMINUTE;
		break;

	case CM_MEDIA_COMPOSER:
		return IDS_COLUMN_NAME_COMPOSER;
		break;

	case CM_MEDIA_CONDUCTOR:
		return IDS_COLUMN_NAME_CONDUCTOR;
		break;

	case CM_MEDIA_DIRECTOR:
		return IDS_COLUMN_NAME_DIRECTOR;
		break;

	case CM_MEDIA_GENRE:
		return IDS_COLUMN_NAME_GENRE;
		break;

	case CM_MEDIA_LANGUAGE:
		return IDS_COLUMN_NAME_LANGUAGE;
		break;

	case CM_MEDIA_BROADCASTDATE:
		return IDS_COLUMN_NAME_BROADCASTDATE;
		break;

	case CM_MEDIA_CHANNEL:
		return IDS_COLUMN_NAME_CHANNEL;
		break;

	case CM_MEDIA_STATIONNAME:
		return IDS_COLUMN_NAME_STATIONNAME;
		break;

	case CM_MEDIA_MOOD:
		return IDS_COLUMN_NAME_MOOD;
		break;

	case CM_MEDIA_PARENTALRATING:
		return IDS_COLUMN_NAME_PARENTALRATING;
		break;

	case CM_MEDIA_PARENTALRATINGREASON:
		return IDS_COLUMN_NAME_PARENTALRATINGREASON;
		break;

	case CM_MEDIA_PERIOD:
		return IDS_COLUMN_NAME_PERIOD;
		break;

	case CM_MEDIA_PRODUCER:
		return IDS_COLUMN_NAME_PRODUCER;
		break;

	case CM_MEDIA_PUBLISHER:
		return IDS_COLUMN_NAME_PUBLISHER;
		break;

	case CM_MEDIA_WRITER:
		return IDS_COLUMN_NAME_WRITER;
		break;

	case CM_MEDIA_YEAR:
		return IDS_COLUMN_NAME_YEAR;
		break;
	}

	return 0;
}

int CContainer::LookupColumnDescriptionStringIndex(int iColumnId)
{
	switch(iColumnId)
	{
		case CM_NAME:
			return IDS_COLUMN_DESCRIPTION_NAME;
			break;

		case CM_TYPE:
			return IDS_COLUMN_DESCRIPTION_TYPE;
			break;

		case CM_SIZE:
			return IDS_COLUMN_DESCRIPTION_SIZE;
			break;

		case CM_DATEMODIFIED:
			return IDS_COLUMN_DESCRIPTION_MODIFIED;
			break;

		case CM_ATTRIBUTES:
			return IDS_COLUMN_DESCRIPTION_ATTRIBUTES;
			break;

		case CM_REALSIZE:
			return IDS_COLUMN_DESCRIPTION_REALSIZE;
			break;

		case CM_SHORTNAME:
			return IDS_COLUMN_DESCRIPTION_SHORTNAME;
			break;

		case CM_OWNER:
			return IDS_COLUMN_DESCRIPTION_OWNER;
			break;

		case CM_PRODUCTNAME:
			return IDS_COLUMN_DESCRIPTION_PRODUCTNAME;
			break;

		case CM_COMPANY:
			return IDS_COLUMN_DESCRIPTION_COMPANY;
			break;

		case CM_DESCRIPTION:
			return IDS_COLUMN_DESCRIPTION_DESCRIPTION;
			break;

		case CM_FILEVERSION:
			return IDS_COLUMN_DESCRIPTION_FILEVERSION;
			break;

		case CM_PRODUCTVERSION:
			return IDS_COLUMN_DESCRIPTION_PRODUCTVERSION;
			break;

		case CM_SHORTCUTTO:
			return IDS_COLUMN_DESCRIPTION_SHORTCUTTO;
			break;

		case CM_HARDLINKS:
			return IDS_COLUMN_DESCRIPTION_HARDLINKS;
			break;

		case CM_EXTENSION:
			return IDS_COLUMN_DESCRIPTION_EXTENSION;
			break;
			
		case CM_CREATED:
			return IDS_COLUMN_DESCRIPTION_CREATED;
			break;
			
		case CM_ACCESSED:
			return IDS_COLUMN_DESCRIPTION_ACCESSED;
			break;

		case CM_TITLE:
			return IDS_COLUMN_DESCRIPTION_TITLE;
			break;

		case CM_SUBJECT:
			return IDS_COLUMN_DESCRIPTION_SUBJECT;
			break;

		case CM_AUTHOR:
			return IDS_COLUMN_DESCRIPTION_AUTHOR;
			break;

		case CM_KEYWORDS:
			return IDS_COLUMN_DESCRIPTION_KEYWORDS;
			break;

		case CM_COMMENT:
			return IDS_COLUMN_DESCRIPTION_COMMENT;
			break;

		case CM_CAMERAMODEL:
			return IDS_COLUMN_DESCRIPTION_CAMERAMODEL;
			break;

		case CM_DATETAKEN:
			return IDS_COLUMN_DESCRIPTION_DATETAKEN;
			break;

		case CM_WIDTH:
			return IDS_COLUMN_DESCRIPTION_WIDTH;
			break;

		case CM_HEIGHT:
			return IDS_COLUMN_DESCRIPTION_HEIGHT;
			break;

		case CM_VIRTUALCOMMENTS:
			return IDS_COLUMN_DESCRIPTION_COMMENT;
			break;

		case CM_TOTALSIZE:
			return IDS_COLUMN_DESCRIPTION_TOTALSIZE;
			break;

		case CM_FREESPACE:
			return IDS_COLUMN_DESCRIPTION_FREESPACE;
			break;

		case CM_FILESYSTEM:
			return IDS_COLUMN_DESCRIPTION_FILESYSTEM;
			break;

		case CM_VIRTUALTYPE:
			return IDS_COLUMN_DESCRIPTION_TYPE;
			break;

		case CM_NUMPRINTERDOCUMENTS:
			return IDS_COLUMN_DESCRIPTION_NUMPRINTERDOCUMENTS;
			break;

		case CM_PRINTERCOMMENTS:
			return IDS_COLUMN_DESCRIPTION_PRINTERCOMMENTS;
			break;

		case CM_PRINTERLOCATION:
			return IDS_COLUMN_DESCRIPTION_PRINTERLOCATION;
			break;

		case CM_NETWORKADAPTER_STATUS:
			return IDS_COLUMN_DESCRIPTION_NETWORKADAPTER_STATUS;
			break;

		case CM_MEDIA_BITRATE:
			return IDS_COLUMN_DESCRIPTION_BITRATE;
			break;
	}

	return 0;
}