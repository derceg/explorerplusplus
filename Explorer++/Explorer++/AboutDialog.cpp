/******************************************************************
 *
 * Project: Explorer++
 * File: AboutDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles all messages associated with the 'About' dialog box.
 *
 * Notes:
 *  - Center dialog. Don't remember previous position.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "Misc.h"
#include "Explorer++.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/Controls.h"
#include "../Helper/Bookmark.h"
#include "MainResource.h"

#ifndef _DEBUG
	#include "Version.h"
#endif


LRESULT	CALLBACK AboutStaticProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
LRESULT	(CALLBACK *DefaultAboutStaticProc)(HWND,UINT,WPARAM,LPARAM);

RECT	rcWebsiteLink = {0,0,0,0};
BOOL	bOverUrl = FALSE;

INT_PTR CALLBACK AboutDialogProcedure(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				HWND	hStaticImage;
				TCHAR	szVersion[64];
				TCHAR	szTemp[64];
				TCHAR	szBuild[64];
				TCHAR	szBuildDate[64];
				HFONT	hFont;
				LOGFONT	Logfont;

				hFont = (HFONT)SendMessage(hDlg,WM_GETFONT,NULL,NULL);
				GetObject(hFont,sizeof(Logfont),&Logfont);

				Logfont.lfWeight = FW_BOLD;
				hFont = CreateFontIndirect(&Logfont);

				SendMessage(hDlg,WM_SETICON,ICON_SMALL,(LPARAM)LoadImage(GetModuleHandle(0),MAKEINTRESOURCE(IDI_MAIN),IMAGE_ICON,32,32,LR_VGACOLOR));

				hStaticImage = GetDlgItem(hDlg,IDC_ABOUT_STATIC_IMAGE);

				DefaultAboutStaticProc = (WNDPROC)SetWindowLongPtr(hStaticImage,GWLP_WNDPROC,(LONG_PTR)AboutStaticProc);

				/* Indicate which architecture (32-bit or
				64-bit) we are building for in the version
				string.*/
				#ifdef WIN64
				StringCchCopy(szBuild,SIZEOF_ARRAY(szBuild),_T("64-bit"));
				#else
				StringCchCopy(szBuild,SIZEOF_ARRAY(szBuild),_T("32-bit"));
				#endif

				#ifdef UNICODE
				StringCchCat(szBuild,SIZEOF_ARRAY(szBuild),_T(" Unicode build"));
				#else
				StringCchCat(szBuild,SIZEOF_ARRAY(szBuild),_T(" build"));
				#endif

				GetDlgItemText(hDlg,IDC_STATIC_VERSIONNUMBER,szTemp,SIZEOF_ARRAY(szTemp));
				StringCchPrintf(szVersion,SIZEOF_ARRAY(szVersion),szTemp,VERSION_NUMBER,szBuild);

				SetDlgItemText(hDlg,IDC_STATIC_VERSIONNUMBER,szVersion);

				/* We'll only show a build date in non-debug mode. */
				#ifndef _DEBUG
					GetDlgItemText(hDlg,IDC_STATIC_BUILDDATE,szTemp,SIZEOF_ARRAY(szTemp));
					StringCchPrintf(szBuildDate,SIZEOF_ARRAY(szBuildDate),szTemp,VERSION_BUILD_DATE);
				#else
					StringCchCopy(szBuildDate,SIZEOF_ARRAY(szBuildDate),_T("[Debug Build]"));
				#endif

				SetDlgItemText(hDlg,IDC_STATIC_BUILDDATE,szBuildDate);

				CenterWindow(GetParent(hDlg),hDlg);
			}
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
					EndDialog(hDlg,1);
					break;

				case IDCANCEL:
					EndDialog(hDlg,0);
					break;
			}
			break;

		case WM_NOTIFY:
			{
				switch(((LPNMHDR)lParam)->code)
				{
				case NM_CLICK:
				case NM_RETURN:
					{
						PNMLINK pNMLink = NULL;

						pNMLink = (PNMLINK)lParam;

						if(((LPNMHDR)lParam)->hwndFrom == GetDlgItem(hDlg,IDC_SITELINK))
						{
							ShellExecute(NULL,L"open",pNMLink->item.szUrl,NULL,NULL,SW_SHOW);
						}
					}
					break;
				}
			}
			break;

		case WM_CLOSE:
			EndDialog(hDlg,0);
			break;
	}

	return 0;
}

LRESULT CALLBACK AboutStaticProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
		case WM_PAINT:
			{
				PAINTSTRUCT	ps;
				HWND		hStaticImage;
				HBITMAP		hbAbout;
				HBITMAP		hbOld;
				HDC			hdc;
				HDC			hdcSrc;
				RECT		rcUpdate;
				RECT		rcStaticImage;

				hStaticImage = hwnd;

				if(GetUpdateRect(hStaticImage,&rcUpdate,FALSE) == 0)
					return 0;

				GetClientRect(hStaticImage,&rcStaticImage);

				hdc = BeginPaint(hStaticImage,&ps);

				hdcSrc = CreateCompatibleDC(hdc);

				hbAbout = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_ABOUT));

				hbOld = (HBITMAP)SelectObject(hdcSrc,hbAbout);

				BitBlt(hdc,ps.rcPaint.left,ps.rcPaint.top,GetRectWidth(&ps.rcPaint),
					GetRectHeight(&ps.rcPaint),hdcSrc,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);

				SelectObject(hdcSrc,hbOld);
				DeleteObject(hbAbout);
				DeleteDC(hdcSrc);

				EndPaint(hStaticImage,&ps);

				return 0;
			}
			break;
	}

	return CallWindowProc(DefaultAboutStaticProc,hwnd,msg,wParam,lParam);
}