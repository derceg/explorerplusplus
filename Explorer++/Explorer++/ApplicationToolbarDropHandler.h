#ifndef APPLICATIONTOOLBARDROPHANDLER_INCLUDED
#define APPLICATIONTOOLBARDROPHANDLER_INCLUDED

#include "ApplicationToolbar.h"

class CApplicationToolbar;

class CApplicationToolbarDropHandler : public IDropTarget
{
public:

	CApplicationToolbarDropHandler(HWND hToolbar, CApplicationToolbar *toolbar);
	~CApplicationToolbarDropHandler();

	/* IUnknown methods. */
	HRESULT __stdcall	QueryInterface(REFIID iid,void **ppvObject);
	ULONG __stdcall		AddRef(void);
	ULONG __stdcall		Release(void);

	/* Drag and drop. */
	HRESULT _stdcall	DragEnter(IDataObject *pDataObject,DWORD grfKeyStat,POINTL pt,DWORD *pdwEffect);
	HRESULT _stdcall	DragOver(DWORD grfKeyState,POINTL pt,DWORD *pdwEffect);
	HRESULT _stdcall	DragLeave(void);
	HRESULT _stdcall	Drop(IDataObject *pDataObject,DWORD grfKeyState,POINTL ptl,DWORD *pdwEffect);

private:

	static FORMATETC	GetSupportedDropFormat();
	void				AddNewButton(DROPFILES *df);
	void				OpenExistingButton(DROPFILES *df, int buttonIndex);

	ULONG				m_RefCount;

	CApplicationToolbar	*m_toolbar;
	HWND				m_hToolbar;

	/* Drag and drop. */
	IDragSourceHelper	*m_pDragSourceHelper;
	IDropTargetHelper	*m_pDropTargetHelper;
};

#endif