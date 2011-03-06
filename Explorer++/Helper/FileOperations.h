#ifndef FILEOPERATIONS_INCLUDED
#define FILEOPERATIONS_INCLUDED

#include <list>

enum OverwriteMethod_t
{
	OVERWRITE_ONEPASS	= 1,
	OVERWRITE_THREEPASS	= 2
};

/* Renaming. */
BOOL	RenameFile(std::wstring strOldFilename,std::wstring strNewFilename);

/* Deletion. */
BOOL	DeleteFiles(HWND hwnd,const std::list<std::wstring> &FullFilenameList,BOOL Permanent);
void	DeleteFileSecurely(TCHAR *szFileName,OverwriteMethod_t OverwriteMethod);

/* Copy and cut. */
HRESULT	CopyFilesToClipboard(std::list<std::wstring> FileNameList,BOOL bMove,IDataObject **pClipboardDataObject);
HRESULT	CopyFiles(std::list<std::wstring> FileNameList,IDataObject **pClipboardDataObject);
HRESULT	CutFiles(std::list<std::wstring> FileNameList,IDataObject **pClipboardDataObject);

/* General. */
int		CreateQualifiedPathName(TCHAR *,TCHAR *,unsigned int);
HRESULT	CreateNewFolder(TCHAR *Directory,TCHAR *szNewFolderName,int cchMax);
HRESULT	SaveDirectoryListing(TCHAR *,TCHAR *);
BOOL	PerformFileOperation(HWND,TCHAR *,TCHAR *,TCHAR *,TCHAR *);

#endif