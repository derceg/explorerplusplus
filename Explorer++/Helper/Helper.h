#pragma once

#include <windows.h>
#include <winioctl.h>
#include <list>
#include <ShObjIdl.h>

/* Major version numbers for various versions of
Windows. */
#define WINDOWS_VISTA_SEVEN_MAJORVERSION	6
#define WINDOWS_XP_MAJORVERSION				5

/* The following declarations, relating to
ICustomDestinationList, are only valid for
Windows 7. If building for an earlier
version of Windows, these declarations
will be required.
All three declarations are from
ShObjIdl.h. */
typedef /* [v1_enum] */ 
enum KNOWNDESTCATEGORY
	{	KDC_FREQUENT	= 1,
	KDC_RECENT	= ( KDC_FREQUENT + 1 ) 
	} 	KNOWNDESTCATEGORY;

EXTERN_C const IID IID_ICustomDestinationList;

MIDL_INTERFACE("6332debf-87b5-4670-90c0-5e57b408a49e")
ICustomDestinationList : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE SetAppID( 
		/* [string][in] */ __RPC__in_string LPCWSTR pszAppID) = 0;

	virtual HRESULT STDMETHODCALLTYPE BeginList( 
		/* [out] */ __RPC__out UINT *pcMinSlots,
		/* [in] */ __RPC__in REFIID riid,
		/* [iid_is][out] */ __RPC__deref_out_opt void **ppv) = 0;

	virtual HRESULT STDMETHODCALLTYPE AppendCategory( 
		/* [string][in] */ __RPC__in_string LPCWSTR pszCategory,
		/* [in] */ __RPC__in_opt IObjectArray *poa) = 0;

	virtual HRESULT STDMETHODCALLTYPE AppendKnownCategory( 
		/* [in] */ KNOWNDESTCATEGORY category) = 0;

	virtual HRESULT STDMETHODCALLTYPE AddUserTasks( 
		/* [in] */ __RPC__in_opt IObjectArray *poa) = 0;

	virtual HRESULT STDMETHODCALLTYPE CommitList( void) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetRemovedDestinations( 
		/* [in] */ __RPC__in REFIID riid,
		/* [iid_is][out] */ __RPC__deref_out_opt void **ppv) = 0;

	virtual HRESULT STDMETHODCALLTYPE DeleteList( 
		/* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszAppID) = 0;

	virtual HRESULT STDMETHODCALLTYPE AbortList( void) = 0;

};

/* SummaryInformation stream constants. */
#define PROPERTY_ID_TITLE		0x02
#define PROPERTY_ID_SUBJECT		0x03
#define PROPERTY_ID_AUTHOR		0x04
#define PROPERTY_ID_KEYWORDS	0x05
#define PROPERTY_ID_COMMENT		0x06

typedef struct
{
	WORD wLanguage;
	WORD wCodePage;
} LANGANDCODEPAGE;

typedef enum
{
	GROUP_ADMINISTRATORS,
	GROUP_POWERUSERS,
	GROUP_USERS,
	GROUP_USERSRESTRICTED
} GroupType_t;

enum SizeDisplayFormat_t
{
	SIZE_FORMAT_NONE,
	SIZE_FORMAT_BYTES,
	SIZE_FORMAT_KBYTES,
	SIZE_FORMAT_MBYTES,
	SIZE_FORMAT_GBYTES,
	SIZE_FORMAT_TBYTES,
	SIZE_FORMAT_PBYTES
};

/* Tab control helper functions. */
void			TabCtrl_SwapItems(HWND hTabCtrl,int iItem1,int iItem2);
void			TabCtrl_SetItemText(HWND,int,TCHAR *);

/* File helpers. */
int				CreateFileTimeString(const FILETIME *FileTime,TCHAR *Buffer,int MaxCharacters,BOOL bFriendlyDate);
BOOL			GetRealFileSize(const std::wstring &strFilename,PLARGE_INTEGER lpRealFileSize);
BOOL			CompareFileTypes(const TCHAR *pszFile1,const TCHAR *pszFile2);
DWORD			BuildFileAttributeString(const TCHAR *lpszFileName,TCHAR *Buffer,DWORD BufSize);
void			BuildFileAttributeStringInternal(DWORD dwFileAttributes,TCHAR *szOutput,DWORD cchMax);
size_t			GetFileOwner(const TCHAR *szFile,TCHAR *szOwner,DWORD BufSize);
DWORD			GetNumFileHardLinks(const TCHAR *lpszFileName);
int				ReadFileProperty(const TCHAR *lpszFileName,DWORD dwPropertyId,TCHAR *lpszPropertyBuf,DWORD dwBufLen);
BOOL			GetVersionInfoString(const TCHAR *szFileName,const TCHAR *szVersionInfo,TCHAR *szBuffer,UINT cbBufLen);
BOOL			ReadImageProperty(const TCHAR *lpszImage,UINT PropertyId,void *pPropBuffer,DWORD dwBufLen);
BOOL			IsImage(const TCHAR *FileName);
WORD			GetFileLanguage(const TCHAR *szFullFileName);
BOOL			GetFileProductVersion(const TCHAR *szFullFileName,DWORD *pdwProductVersionLS,DWORD *pdwProductVersionMS);

/* Ownership and access. */
BOOL			GetProcessOwner(TCHAR *szOwner,DWORD BufSize);
BOOL			CheckGroupMembership(GroupType_t GroupType);

/* Menu helpers. */
BOOL			lCheckMenuItem(HMENU hMenu,UINT ItemID,BOOL bCheck);
BOOL			lEnableMenuItem(HMENU hMenu,UINT ItemID,BOOL bEnable);

/* Time helpers. */
BOOL			LocalSystemTimeToFileTime(const LPSYSTEMTIME lpLocalTime,LPFILETIME lpFileTime);
BOOL			FileTimeToLocalSystemTime(const LPFILETIME lpFileTime,LPSYSTEMTIME lpLocalTime);

/* File slack. */
int				ReadFileSlack(const TCHAR *FileName,TCHAR *pszSlack,int iBufferLen);

/* User interaction. */
BOOL			GetFileNameFromUser(HWND hwnd,TCHAR *FullFileName,const TCHAR *InitialDirectory);

/* Device related. */
TCHAR			*DecodePrinterStatus(DWORD dwStatus);

/* Process helpers. */
DWORD			GetCurrentProcessImageName(TCHAR *szImageName,DWORD nSize);

/* Computer information. */
void			GetCPUBrandString(char *pszCPUBrand,UINT cchBuf);

/* Media metadata. */
HRESULT			GetMediaMetadata(const TCHAR *szFileName,const TCHAR *szAttribute,BYTE **pszOutput);

/* General helper functions. */
void			FormatSizeString(ULARGE_INTEGER lFileSize,TCHAR *pszFileSize,size_t cchBuf);
void			FormatSizeString(ULARGE_INTEGER lFileSize,TCHAR *pszFileSize,size_t cchBuf,BOOL bForceSize,SizeDisplayFormat_t sdf);
HINSTANCE		StartCommandPrompt(const TCHAR *Directory,bool Elevated);
BOOL			SetProcessTokenPrivilege(DWORD ProcessId,const TCHAR *PrivilegeName,BOOL bEnablePrivilege);
int				GetRectHeight(const RECT *rc);
int				GetRectWidth(const RECT *rc);
BOOL			lShowWindow(HWND hwnd,BOOL bShowWindow);
TCHAR			*PrintComma(unsigned long nPrint);
TCHAR			*PrintCommaLargeNum(LARGE_INTEGER lPrint);
BOOL			CheckWildcardMatch(const TCHAR *szWildcard,const TCHAR *szString,BOOL bCaseSensitive);
BOOL			CheckWildcardMatchInternal(const TCHAR *szWildcard,const TCHAR *szString,BOOL bCaseSensitive);
void			ReplaceCharacters(TCHAR *str,char ch,char replacement);
TCHAR			*GetToken(TCHAR *ptr,TCHAR *Buffer,TCHAR *BufferLength);
void			AddGripperStyle(UINT *fStyle,BOOL bAddGripper);
void			AddWindowStyle(HWND hwnd,UINT fStyle,BOOL bAdd);
void			ReplaceCharacterWithString(const TCHAR *szBaseString,TCHAR *szOutput,UINT cchMax,TCHAR chToReplace,const TCHAR *szReplacement);
void			CenterWindow(HWND hParent,HWND hChild);
void			UpdateToolbarBandSizing(HWND hRebar,HWND hToolbar);
void			MergeDateTime(SYSTEMTIME *pstOutput,const SYSTEMTIME *pstDate,const SYSTEMTIME *pstTime);
void			GetWindowString(HWND hwnd,std::wstring &str);
BOOL			lCheckDlgButton(HWND hDlg,int ButtonId,BOOL bCheck);
void			TrimStringLeft(std::wstring &str,const std::wstring &strWhitespace);
void			TrimStringRight(std::wstring &str,const std::wstring &strWhitespace);
void			TrimString(std::wstring &str,const std::wstring &strWhitespace);
void			AddStyleToToolbar(UINT *fStyle,UINT fStyleToAdd);