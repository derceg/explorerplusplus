#ifndef DISPLAY_INCLUDED
#define DISPLAY_INCLUDED

#include <windows.h>
#include <unknwn.h>
#include <gdiplus.h>
#include <vector>

using namespace Gdiplus;
using namespace std;

#define DWM_BASE				(WM_APP + 100)

#define DWM_SETTHUMBNAILFILE	(DWM_BASE + 2)
#define DWM_GETSURROUNDCOLOR	(DWM_BASE + 3)
#define DWM_SETSURROUNDCOLOR	(DWM_BASE + 4)
#define DWM_GETCENTRECOLOR		(DWM_BASE + 7)
#define DWM_SETCENTRECOLOR		(DWM_BASE + 8)
#define DWM_DRAWGRADIENTFILL	(DWM_BASE + 10)
#define DWM_GETFONT				(DWM_BASE + 11)
#define DWM_SETFONT				(DWM_BASE + 12)
#define DWM_GETTEXTCOLOR		(DWM_BASE + 13)
#define DWM_SETTEXTCOLOR		(DWM_BASE + 14)
#define DWM_BUFFERTEXT			(DWM_BASE + 15)
#define DWM_CLEARTEXTBUFFER		(DWM_BASE + 16)
#define DWM_SETLINE				(DWM_BASE + 17)

#define DisplayWindow_SetThumbnailFile(hDisplay,FileName,bShowImage) \
SendMessage(hDisplay,DWM_SETTHUMBNAILFILE,(WPARAM)FileName,(LPARAM)bShowImage)

#define DisplayWindow_GetSurroundColor(hDisplay) \
SendMessage(hDisplay,DWM_GETSURROUNDCOLOR,0,0)

#define DisplayWindow_SetColors(hDisplay,rgbColor) \
SendMessage(hDisplay,DWM_SETSURROUNDCOLOR,rgbColor,0)

#define DisplayWindow_SetFont(hDisplay,hFont) \
SendMessage(hDisplay,DWM_SETFONT,hFont,0)

#define DisplayWindow_GetFont(hDisplay,hFont) \
SendMessage(hDisplay,DWM_GETFONT,hFont,0)

#define DisplayWindow_GetTextColor(hDisplay) \
(COLORREF)SendMessage(hDisplay,DWM_GETTEXTCOLOR,0,0)

#define DisplayWindow_SetTextColor(hDisplay,hColor) \
SendMessage(hDisplay,DWM_SETTEXTCOLOR,hColor,0)

#define DisplayWindow_SaveSettings(hDisplay,szKeyPath) \
SendMessage(hDisplay,DWM_SAVESETTINGS,szKeyPath,0)

#define DisplayWindow_LoadSettings(hDisplay,szKeyPath) \
SendMessage(hDisplay,DWM_LOADSETTINGS,szKeyPath,0)

#define DisplayWindow_DirectoryEntered(hDisplay,szDirectory) \
SendMessage(hDisplay,DWM_DIRECTORYENTERED,0,szDirectory)

#define DisplayWindow_BufferText(hDisplay,szText) \
SendMessage(hDisplay,DWM_BUFFERTEXT,(WPARAM)0,(LPARAM)szText)

#define DisplayWindow_ClearTextBuffer(hDisplay) \
SendMessage(hDisplay,DWM_CLEARTEXTBUFFER,(WPARAM)0,(LPARAM)0)

#define DisplayWindow_SetLine(hDisplay,iLine,szText) \
SendMessage(hDisplay,DWM_SETLINE,(WPARAM)iLine,(LPARAM)szText)

#define WM_USER_DISPLAYWINDOWRESIZED	(WM_APP + 100)

#define WM_NDW_ICONRCLICK	(WM_APP + 101)
#define WM_NDW_RCLICK		(WM_APP + 102)

typedef struct
{
	Color		CentreColor;
	Color		SurroundColor;
	COLORREF	TextColor;
	HFONT		hFont;
	HICON		hIcon;
} DWInitialSettings_t;

typedef struct
{
	TCHAR szText[512];
} LineData_t;

typedef struct
{
	void *pdw;
	BOOL bCancelled;
} ThumbnailEntry_t;

__interface IDisplayWindowMain : IUnknown
{
	virtual void DrawGradientFillExternal(HDC hdc,RECT *rc,RECT *UpdateRect);
};

static int g_ObjectCount = 0;

class CDisplayWindow : public IDisplayWindowMain
{
public:

	CDisplayWindow(HWND hDisplayWindow,
	DWInitialSettings_t *pInitialSettings);
	~CDisplayWindow();

	/* IUnknown methods. */
	HRESULT __stdcall QueryInterface(REFIID iid,void **ppvObject);
	ULONG __stdcall	AddRef(void);
	ULONG __stdcall	Release(void);

	LRESULT CALLBACK DisplayWindowProc(HWND,UINT,WPARAM,LPARAM);

	/* Used for previewing colour changes in the window. */
	void	DrawGradientFillExternal(HDC hdc,RECT *rc,RECT *UpdateRect);

	void	ExtractThumbnailImageInternal(ThumbnailEntry_t *pte);

private:

	#define BORDER_COLOUR		Color(128,128,128)
	#define NUM_IMAGE_TYPES		9

	typedef struct
	{
		int xLeft;
		int yTop;
		int cxWidth;
		int cyWidth;
	} Icon_t;

	LONG	OnMouseMove(LPARAM lParam);
	void	OnLButtonDown(LPARAM lParam);
	void	OnRButtonUp(WPARAM wParam,LPARAM lParam);
	void	DrawGradientFill(HDC,RECT *,RECT *);
	void	EraseLine(unsigned int);
	void	FlushLineToScreen(HDC,unsigned int);
	void	PaintText(HDC hdc,unsigned int,unsigned int);
	void	TransparentTextOut(HDC hdc,TCHAR *Text,RECT *prcText);
	void	DrawThumbnail(HDC hdcMem);
	void	OnSetThumbnailFile(WPARAM wParam,LPARAM lParam);
	void	OnSetFont(HFONT hFont);
	void	OnSetTextColor(COLORREF hColor);

	void	SetWindowSize(HWND,int);
	void	ApplyDefaultFont(HDC hdc);

	void	PatchBackground(HDC hdc,RECT *rc,RECT *UpdateRect);

	void	OnSize(WPARAM wParam,LPARAM lParam);

	void	ExtractThumbnailImage(void);
	void	CancelThumbnailExtraction(void);




	/* ------ Internal state. ------ */

	/* IUnknown members. */
	int				m_iRefCount;

	HWND			m_hDisplayWindow;

	/* Text drawing attributes. */
	COLORREF		m_TextColor;
	unsigned int	m_LineSpacing;
	unsigned int	m_LeftIndent;
	unsigned int	m_AveTextWidth;

	/* Text buffers (for internal redrawing operations). */
	vector<LineData_t>	m_LineList;
	TCHAR			m_ImageFile[MAX_PATH];
	BOOL			m_bSizing;
	Color			m_CentreColor;
	Color			m_SurroundColor;

	int				m_iImageWidth;
	int				m_iImageHeight;

	/* Thumbnails. */
	CRITICAL_SECTION	m_csDWThumbnails;
	HBITMAP			m_hbmThumbnail;
	BOOL			m_bShowThumbnail;
	BOOL			m_bThumbnailExtracted;
	BOOL			m_bThumbnailExtractionFailed;

	int				m_xColumnFinal;

	Icon_t IconInfo;

	TCHAR ImageTypes[NUM_IMAGE_TYPES][4];
	HDC m_hdcBackground;
	Graphics *m_pFadingImage;
	double m_dCurrentAlpha;
	HDC m_hdcMemFading;
	Image *m_FadingImageThumb;
	HBITMAP m_hBitmapBackground;
	HICON m_hMainIcon;
	HFONT m_hDisplayFont;
	HANDLE m_hFirstCycleImage;
	WIN32_FIND_DATA m_wfdCycle;
};

HWND CreateDisplayWindow(HWND Parent,IDisplayWindowMain **pMain,
DWInitialSettings_t *pSettings);

#endif