#ifndef FILEOPERATIONS_INCLUDED
#define FILEOPERATIONS_INCLUDED

typedef enum
{
	OVERWRITE_ONEPASS	= 1,
	OVERWRITE_THREEPASS	= 2
} OVERWRITE_METHODS;

typedef enum
{
	DRAG_TYPE_LEFTCLICK,
	DRAG_TYPE_RIGHTCLICK
} DragTypes_t;

/* Renaming. */
int		RenameFile(TCHAR *NewFileName,TCHAR *OldFileName);

/* Deletion. */
int		DeleteFiles(HWND,TCHAR *,BOOL);
int		DeleteFilesToRecycleBin(HWND hwnd,TCHAR *FileNameList);
int		DeleteFilesPermanently(HWND hwnd,TCHAR *FileNameList);
void	DeleteFileSecurely(TCHAR *szFileName,UINT uOverwriteMethod);

/* Copy and cut. */
HRESULT CopyFilesToClipboard(TCHAR *FileNameList,size_t iListSize,BOOL bMove,IDataObject **pClipboardDataObject);
HRESULT	CopyFiles(TCHAR *szFileNameList,int iListSize,IDataObject **pClipboardDataObject);
HRESULT	CutFiles(TCHAR *szFileNameList,int iListSize,IDataObject **pClipboardDataObject);

/* General. */
int		CreateQualifiedPathName(TCHAR *,TCHAR *,unsigned int);
HRESULT	CreateNewFolder(TCHAR *Directory,TCHAR *szNewFolderName,int cchMax);
HRESULT	SaveDirectoryListing(TCHAR *,TCHAR *);
void	GetDeletedFileDate(TCHAR *szFileName,FILETIME *pFileTime,TCHAR *szOriginalLocation);
BOOL	PerformFileOperation(HWND,TCHAR *,TCHAR *,TCHAR *,TCHAR *);
void	CountFilesAndFolders(TCHAR *,int *,int *,double *);
BOOL	ShowFileProperties(TCHAR *);

#endif