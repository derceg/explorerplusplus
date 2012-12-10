#ifndef APPLICATIONTOOLBARDROPHANDLER_INCLUDED
#define APPLICATIONTOOLBARDROPHANDLER_INCLUDED

class CApplicationToolbarDropHandler : public IDropTarget
{
public:

	CApplicationToolbarDropHandler(HWND hToolbar);
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

	ULONG				m_RefCount;

	HWND				m_hToolbar;

	/* Drag and drop. */
	IDragSourceHelper	*m_pDragSourceHelper;
	IDropTargetHelper	*m_pDropTargetHelper;
	BOOL				m_bAcceptData;
};

#endif