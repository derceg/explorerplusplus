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
#include "Version.h"


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
				HFONT	hFont;
				LOGFONT	Logfont;

				hFont = (HFONT)SendMessage(hDlg,WM_GETFONT,NULL,NULL);
				GetObject(hFont,sizeof(Logfont),&Logfont);

				Logfont.lfWeight = FW_BOLD;
				hFont = CreateFontIndirect(&Logfont);

				SendMessage(hDlg,WM_SETICON,ICON_SMALL,(LPARAM)LoadImage(GetModuleHandle(0),MAKEINTRESOURCE(IDI_MAIN),IMAGE_ICON,32,32,LR_VGACOLOR));

				hStaticImage = GetDlgItem(hDlg,IDC_ABOUT_STATIC_IMAGE);

				DefaultAboutStaticProc = (WNDPROC)SetWindowLongPtr(hStaticImage,GWLP_WNDPROC,(LONG_PTR)AboutStaticProc);

				CenterWindow(GetParent(hDlg),hDlg);
			}
			break;

		case WM_MOUSEMOVE:
			{
				HWND	hStaticImage;
				RECT	rcStaticImage;
				POINTS	ptCursor;
				POINT	ptCursorStatic;

				ptCursor = MAKEPOINTS(lParam);

				ptCursorStatic.x	= ptCursor.x;
				ptCursorStatic.y	= ptCursor.y;

				hStaticImage = GetDlgItem(hDlg,IDC_ABOUT_STATIC_IMAGE);

				GetWindowRect(hStaticImage,&rcStaticImage);

				MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rcStaticImage,2);

				if(PtInRect(&rcStaticImage,ptCursorStatic))
				{
					SendMessage(hStaticImage,WM_MOUSEMOVE,wParam,lParam);
				}
			}
			break;

		case WM_LBUTTONDOWN:
			{
				HWND	hStaticImage;
				RECT	rcStaticImage;
				POINTS	ptCursor;
				POINT	ptCursorStatic;

				ptCursor = MAKEPOINTS(lParam);

				ptCursorStatic.x	= ptCursor.x;
				ptCursorStatic.y	= ptCursor.y;

				hStaticImage = GetDlgItem(hDlg,IDC_ABOUT_STATIC_IMAGE);

				GetWindowRect(hStaticImage,&rcStaticImage);

				MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rcStaticImage,2);

				if(PtInRect(&rcStaticImage,ptCursorStatic))
				{
					SendMessage(hStaticImage,WM_LBUTTONDOWN,wParam,lParam);
				}
			}
			break;

		case WM_LBUTTONUP:
			{
				HWND	hStaticImage;
				RECT	rcStaticImage;
				POINTS	ptCursor;
				POINT	ptCursorStatic;

				ptCursor = MAKEPOINTS(lParam);

				ptCursorStatic.x	= ptCursor.x;
				ptCursorStatic.y	= ptCursor.y;

				hStaticImage = GetDlgItem(hDlg,IDC_ABOUT_STATIC_IMAGE);

				GetWindowRect(hStaticImage,&rcStaticImage);

				MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rcStaticImage,2);

				if(PtInRect(&rcStaticImage,ptCursorStatic))
				{
					SendMessage(hStaticImage,WM_LBUTTONUP,wParam,lParam);
				}
			}
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
					EndDialog(hDlg,1);
					break;
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
		case WM_MOUSEMOVE:
			{
				POINTS	ptCursor;
				POINT	ptCursorStatic;

				ptCursor = MAKEPOINTS(lParam);

				ptCursorStatic.x	= ptCursor.x;
				ptCursorStatic.y	= ptCursor.y;

				MapWindowPoints(GetParent(hwnd),hwnd,(LPPOINT)&ptCursorStatic,1);

				if(PtInRect(&rcWebsiteLink,ptCursorStatic))
				{
					bOverUrl = TRUE;
					SetCursor(LoadCursor(NULL,IDC_HAND));
					RedrawWindow(hwnd,&rcWebsiteLink,NULL,RDW_INVALIDATE);
				}
				else
				{
					if(bOverUrl)
					{
						bOverUrl = FALSE;
						SetCursor(LoadCursor(NULL,IDC_ARROW));
						RedrawWindow(hwnd,&rcWebsiteLink,NULL,RDW_INVALIDATE);
					}
				}
			}
			break;

		case WM_LBUTTONDOWN:
			{
				POINTS	ptCursor;
				POINT	ptCursorStatic;

				ptCursor = MAKEPOINTS(lParam);

				ptCursorStatic.x	= ptCursor.x;
				ptCursorStatic.y	= ptCursor.y;

				MapWindowPoints(GetParent(hwnd),hwnd,(LPPOINT)&ptCursorStatic,1);

				if(PtInRect(&rcWebsiteLink,ptCursorStatic))
				{
					SetCursor(LoadCursor(NULL,IDC_HAND));
				}

				return 0;
			}
			break;

		case WM_LBUTTONUP:
			{
				POINTS	ptCursor;
				POINT	ptCursorStatic;

				ptCursor = MAKEPOINTS(lParam);

				ptCursorStatic.x	= ptCursor.x;
				ptCursorStatic.y	= ptCursor.y;

				MapWindowPoints(GetParent(hwnd),hwnd,(LPPOINT)&ptCursorStatic,1);

				if(PtInRect(&rcWebsiteLink,ptCursorStatic))
				{
					SHELLEXECUTEINFO sei;

					sei.cbSize			= sizeof(sei);
					sei.fMask			= 0;
					sei.hwnd			= NULL;
					sei.lpVerb			= _T("open");
					sei.lpFile			= WEBSITE_URL;
					sei.lpParameters	= NULL;
					sei.lpDirectory		= NULL;
					ShellExecuteEx(&sei);

					SetCursor(LoadCursor(NULL,IDC_HAND));
				}

				return 0;
			}
			break;

		case WM_PAINT:
			{
				PAINTSTRUCT	ps;
				HWND		hStaticImage;
				HBITMAP		hbAbout;
				HBITMAP		hbOld;
				HDC			hdc;
				HDC			hdcSrc;
				HFONT		hFont;
				HFONT		hOldFont;
				RECT		rcUpdate;
				RECT		rcStaticImage;
				RECT		rcVersionText;
				RECT		rcBuildDateText;
				SIZE		szWebsiteLinkText;

				hStaticImage = hwnd;

				if(GetUpdateRect(hStaticImage,&rcUpdate,FALSE) == 0)
					return 0;

				GetClientRect(hStaticImage,&rcStaticImage);

				hdc = BeginPaint(hStaticImage,&ps);

				hdcSrc = CreateCompatibleDC(hdc);

				SetBkMode(hdcSrc,TRANSPARENT);

				hFont = CreateFont(20,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,
					OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_ROMAN,
					_T("Centaur"));

				hOldFont = (HFONT)SelectObject(hdcSrc,hFont);

				hbAbout = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_ABOUT));

				hbOld = (HBITMAP)SelectObject(hdcSrc,hbAbout);

				rcVersionText = rcStaticImage;

				TCHAR szTemp[32];

				LoadString(g_hLanguageModule,IDS_VERSION,szTemp,SIZEOF_ARRAY(szTemp));

				TCHAR szVersion[64];

				StringCchPrintf(szVersion,SIZEOF_ARRAY(szVersion),_T("%s %s"),szTemp,VERSION_NUMBER);

				/* If we are building for 64-bit architecture,
				indicate this in the version string. */
				#ifdef WIN64
				StrCat(szVersion,_T(" (64-bit"));
				#else
				StrCat(szVersion,_T(" (32-bit"));
				#endif

				#ifdef UNICODE
				StrCat(szVersion,_T(" Unicode build)"));
				#else
				StrCat(szVersion,_T(" build)"));
				#endif

				SIZE VersionSize;

				GetTextExtentPoint32(hdcSrc,szVersion,lstrlen(szVersion),&VersionSize);

				OffsetRect(&rcVersionText,185 - (VersionSize.cx / 2),50);

				DrawText(hdcSrc,szVersion,lstrlen(szVersion),
					&rcVersionText,DT_LEFT);

				SelectObject(hdcSrc,hOldFont);
				DeleteFont(hFont);

				hFont = CreateFont(14,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,
					OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH | FF_ROMAN,
					_T("Calligraph421 BT"));

				hOldFont = (HFONT)SelectObject(hdcSrc,hFont);

				rcBuildDateText = rcVersionText;

				SIZE DateSize;
				double xOffset;

				/* The date text will be centered against the version text. */
				GetTextExtentPoint32(hdcSrc,VERSION_BUILD_DATE,lstrlen(VERSION_BUILD_DATE),&DateSize);

				xOffset = (DateSize.cx - VersionSize.cx) / 2.0;

				if(xOffset > 0)
					xOffset = ceil(xOffset);
				else
					xOffset = floor(xOffset);

				OffsetRect(&rcBuildDateText,(int)-xOffset - 10,20);

				/* <---- Build date text ----> */
				DrawText(hdcSrc,VERSION_BUILD_DATE,lstrlen(VERSION_BUILD_DATE),
					&rcBuildDateText,DT_LEFT);

				SelectObject(hdcSrc,hOldFont);
				DeleteFont(hFont);

				if(bOverUrl)
				{
					hFont = CreateFont(20,0,0,0,FW_NORMAL,FALSE,TRUE,FALSE,ANSI_CHARSET,
						OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH | FF_ROMAN,
						_T("Calligraph421 BT"));

					SetTextColor(hdcSrc,RGB(0,0,255));
				}
				else
				{
					hFont = CreateFont(20,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,
						OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH | FF_ROMAN,
						_T("Calligraph421 BT"));
				}

				hOldFont = (HFONT)SelectObject(hdcSrc,hFont);

				GetTextExtentPoint32(hdcSrc,WEBSITE_URL_TEXT,lstrlen(WEBSITE_URL_TEXT),&szWebsiteLinkText);

				rcWebsiteLink = rcStaticImage;
				OffsetRect(&rcWebsiteLink,90,85);

				rcWebsiteLink.right		= rcWebsiteLink.left + szWebsiteLinkText.cx;
				rcWebsiteLink.bottom	= rcWebsiteLink.top + szWebsiteLinkText.cy;

				DrawText(hdcSrc,WEBSITE_URL_TEXT,lstrlen(WEBSITE_URL_TEXT),
					&rcWebsiteLink,DT_LEFT);

				BitBlt(hdc,ps.rcPaint.left,ps.rcPaint.top,GetRectWidth(&ps.rcPaint),
					GetRectHeight(&ps.rcPaint),hdcSrc,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);

				SelectObject(hdcSrc,hOldFont);
				DeleteFont(hFont);

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