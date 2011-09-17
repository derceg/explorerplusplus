#ifndef ISHELLVIEW2_INCLUDED
#define ISHELLVIEW2_INCLUDED

#include "Explorer++_internal.h"

class CShellView2 : public IShellView2
{
public:

	CShellView2(IExplorerplusplus *pexpp);
	~CShellView2();

private:

	/* IUnknown methods. */
	HRESULT __stdcall	QueryInterface(REFIID iid,void **ppvObject);
	ULONG __stdcall		AddRef(void);
	ULONG __stdcall		Release(void);

	/* IShellView2 methods. */
	HRESULT __stdcall	CreateViewWindow2(LPSV2CVW2_PARAMS lpParams);
	HRESULT __stdcall	GetView(SHELLVIEWID *pvid,ULONG uView);
	HRESULT __stdcall	HandleRename(LPCITEMIDLIST pidlNew);
	HRESULT __stdcall	SelectAndPositionItem(LPCITEMIDLIST pidlItem,UINT uFlags,POINT *ppt);

	/* IShellView methods. */
	HRESULT __stdcall	TranslateAccelerator(MSG *msg);
	HRESULT __stdcall	EnableModeless(BOOL fEnable);
	HRESULT __stdcall	UIActivate(UINT uActivate);
	HRESULT __stdcall	Refresh(void);
	HRESULT __stdcall	CreateViewWindow(IShellView *psvPrevious,LPCFOLDERSETTINGS pfs,IShellBrowser *psb,RECT *prcView,HWND *phWnd);
	HRESULT __stdcall	DestroyViewWindow(void);
	HRESULT __stdcall	GetCurrentInfo(LPFOLDERSETTINGS pfs);
	HRESULT __stdcall	AddPropertySheetPages(DWORD dwReserved,LPFNSVADDPROPSHEETPAGE pfn,LPARAM lparam);
	HRESULT __stdcall	SaveViewState(void);
	HRESULT __stdcall	SelectItem(LPCITEMIDLIST pidlItem,SVSIF uFlags);
	HRESULT __stdcall	GetItemObject(UINT uItem,REFIID riid,void **ppv);

	/* IOleWindow methods. */
	HRESULT __stdcall	GetWindow(HWND *);
	HRESULT __stdcall	ContextSensitiveHelp(BOOL bHelp);

	ULONG				m_RefCount;
	IExplorerplusplus	*m_pexpp;
};

#endif