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

/* Generic drop handler. Handles the following
drop formats:
 - CF_HDROP
 - CF_STR_FILEDESCRIPTOR
*/
class CDropHandler
{
public:

	CDropHandler(IDataObject *pDataObject,DWORD grfKeyState,
	POINTL ptl,DWORD *pdwEffect,HWND hwndDrop,DragTypes_t DragType,
	TCHAR *szDestDirectory,IDropFilesCallback *pDropFilesCallback);
	~CDropHandler();

	void	Drop(void);
	void	Paste(IDataObject *pDataObject,DWORD *pdwEffect,HWND hwndDrop,TCHAR *szDestDirectory);

	DWORD WINAPI	CopyDroppedFilesInternalAsync(LPVOID lpParameter);

private:

	void	HandleLeftClickDrop(void);
	void	HandleRightClickDrop(void);
	void	CopyDroppedFiles(DROPFILES *pdf);
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
};

#endif