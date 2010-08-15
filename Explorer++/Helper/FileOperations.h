#ifndef FILEOPERATIONS_INCLUDED
#define FILEOPERATIONS_INCLUDED

typedef enum
{
	OVERWRITE_ONEPASS	= 1,
	OVERWRITE_THREEPASS	= 2
} OVERWRITE_METHODS;

/* Renaming. */
int		RenameFile(TCHAR *NewFileName,TCHAR *OldFileName);

/* Deletion. */
int		DeleteFiles(HWND,TCHAR *,BOOL);
int		DeleteFilesToRecycleBin(HWND hwnd,TCHAR *FileNameList);
int		DeleteFilesPermanently(HWND hwnd,TCHAR *FileNameList);
void	DeleteFileSecurely(TCHAR *szFileName,UINT uOverwriteMethod);

/* Copy and cut. */
HRESULT	CopyFilesToClipboard(std::list<std::wstring> FileNameList,BOOL bMove,IDataObject **pClipboardDataObject);
HRESULT	CopyFiles(std::list<std::wstring> FileNameList,IDataObject **pClipboardDataObject);
HRESULT	CutFiles(std::list<std::wstring> FileNameList,IDataObject **pClipboardDataObject);

/* General. */
int		CreateQualifiedPathName(TCHAR *,TCHAR *,unsigned int);
HRESULT	CreateNewFolder(TCHAR *Directory,TCHAR *szNewFolderName,int cchMax);
HRESULT	SaveDirectoryListing(TCHAR *,TCHAR *);
BOOL	PerformFileOperation(HWND,TCHAR *,TCHAR *,TCHAR *,TCHAR *);
void	CountFilesAndFolders(TCHAR *,int *,int *,double *);
BOOL	ShowFileProperties(TCHAR *);

#endif