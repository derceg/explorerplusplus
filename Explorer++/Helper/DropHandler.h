#ifndef DROPHANDLER_INCLUDED
#define DROPHANDLER_INCLUDED

#include <list>
#include "Helper.h"
#include "FileOperations.h"
#include "Buffer.h"

using namespace std;

typedef enum
{
	DRAG_TYPE_LEFTCLICK,
	DRAG_TYPE_RIGHTCLICK
} DragTypes_t;

__interface IDropFilesCallback
{
	void OnDropFile(list<PastedFile_t> *ppfl,POINT *ppt);
};

__interface IDropHandler
{
	HRESULT		__stdcall	QueryInterface(REFIID iid, void **ppvObject);
	ULONG		__stdcall	AddRef(void);
	ULONG		__stdcall	Release(void);

	void Drop(IDataObject *pDataObject,DWORD grfKeyState,POINTL ptl,DWORD *pdwEffect,HWND hwndDrop,DragTypes_t DragType,TCHAR *szDestDirectory,IDropFilesCallback *pDropFilesCallback,BOOL bRenameOnCollision);
};

__interface IClipboardHandler
{
	HRESULT		__stdcall	QueryInterface(REFIID iid, void **ppvObject);
	ULONG		__stdcall	AddRef(void);
	ULONG		__stdcall	Release(void);

	void CopyClipboardData(IDataObject *pDataObject,HWND hwndDrop,TCHAR *szDestDirectory,IDropFilesCallback *pDropFilesCallback,BOOL bRenameOnCollision);
};

/* Generic drop handler. Handles the following
drop formats:
 - CF_HDROP
 - CF_STR_FILEDESCRIPTOR
*/
class CDropHandler : public IDropHandler, public IClipboardHandler
{
public:

	CDropHandler();
	~CDropHandler();

	HRESULT		__stdcall	QueryInterface(REFIID iid, void **ppvObject);
	ULONG		__stdcall	AddRef(void);
	ULONG		__stdcall	Release(void);

private:

	void	Drop(IDataObject *pDataObject,DWORD grfKeyState,POINTL ptl,DWORD *pdwEffect,HWND hwndDrop,DragTypes_t DragType,TCHAR *szDestDirectory,IDropFilesCallback *pDropFilesCallback,BOOL bRenameOnCollision);
	void	CopyClipboardData(IDataObject *pDataObject,HWND hwndDrop,TCHAR *szDestDirectory,IDropFilesCallback *pDropFilesCallback,BOOL bRenameOnCollision);

	void	HandleLeftClickDrop(IDataObject *pDataObject,TCHAR *pszDestDirectory,POINTL *pptl);
	void	HandleRightClickDrop(void);
	void	CopyDroppedFiles(DROPFILES *pdf,BOOL bPreferredEffect,DWORD dwPreferredEffect);
	void	CopyDroppedFilesInternal(IBufferManager *pbm,list<PastedFile_t> *pPastedFileList,BOOL bCopy,BOOL bRenameOnCollision);
	void	CreateShortcutToDroppedFile(TCHAR *szFullFileName);
	void	CopyTextToFile(TCHAR *pszDestDirectory,WCHAR *pszText);
	BOOL	CheckItemLocations(int iDroppedItem);

	LONG		m_lRefCount;

	IDropFilesCallback	*m_pDropFilesCallback;

	IDataObject	*m_pDataObject;
	DWORD		m_grfKeyState;
	POINTL		m_ptl;
	DWORD		*m_pdwEffect;
	HWND		m_hwndDrop;
	DragTypes_t	m_DragType;
	TCHAR		*m_szDestDirectory;
	BOOL		m_bRenameOnCollision;
};

typedef struct
{
	SHFILEOPSTRUCT		shfo;
	IDropFilesCallback	*pDropFilesCallback;
	list<PastedFile_t>	*pPastedFileList;
	DWORD				dwEffect;
	POINT				pt;

	IAsyncOperation		*pao;
	HWND				m_hDrop;
	HRESULT				hrCopy;

	TCHAR				*pFrom;
	TCHAR				*pTo;

	CDropHandler		*pDropHandler;
}PastedFilesInfo_t;

#endif