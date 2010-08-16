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
	void Drop(IDataObject *pDataObject,DWORD grfKeyState,POINTL ptl,DWORD *pdwEffect,HWND hwndDrop,DragTypes_t DragType,TCHAR *szDestDirectory,IDropFilesCallback *pDropFilesCallback,BOOL bRenameOnCollision);
};

__interface IClipboardHandler
{
	void CopyClipboardData(IDataObject *pDataObject,HWND hwndDrop,TCHAR *szDestDirectory,IDropFilesCallback *pDropFilesCallback,BOOL bRenameOnCollision);
};

typedef struct
{
	void				*pDropHandler;

	SHFILEOPSTRUCT		shfo;
	IDropFilesCallback	*pDropFilesCallback;
	list<PastedFile_t>	*pPastedFileList;
	DWORD				dwEffect;
	POINT				pt;

	IAsyncOperation		*pao;
}PastedFilesInfo_t;

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

	HRESULT	CopyDroppedFilesInternalAsync(PastedFilesInfo_t *ppfi);

private:

	void	Drop(IDataObject *pDataObject,DWORD grfKeyState,POINTL ptl,DWORD *pdwEffect,HWND hwndDrop,DragTypes_t DragType,TCHAR *szDestDirectory,IDropFilesCallback *pDropFilesCallback,BOOL bRenameOnCollision);
	void	CopyClipboardData(IDataObject *pDataObject,HWND hwndDrop,TCHAR *szDestDirectory,IDropFilesCallback *pDropFilesCallback,BOOL bRenameOnCollision);

	void	HandleLeftClickDrop(IDataObject *pDataObject,TCHAR *pszDestDirectory,POINTL *pptl);
	void	HandleRightClickDrop(void);
	void	CopyDroppedFiles(DROPFILES *pdf,BOOL bPreferredEffect,DWORD dwPreferredEffect);
	void	CopyDroppedFilesInternal(IBufferManager *pbm,list<PastedFile_t> *pPastedFileList,BOOL bCopy,BOOL bRenameOnCollision);
	void	CreateShortcutToDroppedFile(TCHAR *szFullFileName);
	BOOL	CheckItemLocations(int iDroppedItem);

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

#endif