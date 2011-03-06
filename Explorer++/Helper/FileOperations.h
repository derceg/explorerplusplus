#ifndef FILEOPERATIONS_INCLUDED
#define FILEOPERATIONS_INCLUDED

#include <list>

enum OverwriteMethod_t
{
	OVERWRITE_ONEPASS	= 1,
	OVERWRITE_THREEPASS	= 2
};

namespace NFileOperations
{
	BOOL	RenameFile(std::wstring strOldFilename,std::wstring strNewFilename);
	BOOL	DeleteFiles(HWND hwnd,const std::list<std::wstring> &FullFilenameList,BOOL Permanent);

	BOOL	SaveDirectoryListing(std::wstring strDirectory,std::wstring strFilename);

	HRESULT	CreateLinkToFile(std::wstring strTargetFilename,std::wstring strLinkFilename,std::wstring strLinkDescription);
	HRESULT	ResolveLink(HWND hwnd,DWORD fFlags,TCHAR *szLinkFilename,TCHAR *szResolvedPath,int nBufferSize);
};

void	DeleteFileSecurely(TCHAR *szFileName,OverwriteMethod_t OverwriteMethod);

HRESULT	CreateNewFolder(TCHAR *Directory,TCHAR *szNewFolderName,int cchMax);

HRESULT	CopyFiles(std::list<std::wstring> FileNameList,IDataObject **pClipboardDataObject);
HRESULT	CutFiles(std::list<std::wstring> FileNameList,IDataObject **pClipboardDataObject);
HRESULT	CopyFilesToClipboard(std::list<std::wstring> FileNameList,BOOL bMove,IDataObject **pClipboardDataObject);

int		PasteLinksToClipboardFiles(TCHAR *szDestination);
int		PasteHardLinks(TCHAR *szDestination);

BOOL	CreateBrowseDialog(HWND hOwner,TCHAR *Title,TCHAR *PathBuffer,int BufferSize);
BOOL	CreateBrowseDialog(HWND hOwner,TCHAR *Title,LPITEMIDLIST *ppidl);

int		CopyFilesToFolder(HWND hOwner,TCHAR *FileNameList,BOOL bMove);

void	DeleteFileSecurely(TCHAR *szFileName);

#endif