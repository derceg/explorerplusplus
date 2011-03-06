#ifndef HELPER_INCLUDED
#define HELPER_INCLUDED

#include <windows.h>
#include <winioctl.h>
#include <list>

using namespace std;

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

#define MAX_STRING_LENGTH	512

/* SummaryInformation stream constants. */
#define PROPERTY_ID_TITLE		0x02
#define PROPERTY_ID_SUBJECT		0x03
#define PROPERTY_ID_AUTHOR		0x04
#define PROPERTY_ID_KEYWORDS	0x05
#define PROPERTY_ID_COMMENT		0x06

/* DocumentSummaryInformation stream. */
#define PROPERTY_ID_CATEGORY	0x02

#define EXE_INVALID				0x4

/* ShellHelper definitions. */
#define DEFAULT_ICON_FOLDER		0
#define DEFAULT_ICON_FILE		1

/* CP_ACP is defined for ANSI, but there is no
corresponding definition for Unicode. Therefore,
add a definition here. */
#define CP_UNICODE	1200

#define FRIENDLY_NAME_DESKTOP	_T("Desktop")
#define FRIENDLY_NAME_PICTURES	_T("Pictures")
#define FRIENDLY_NAME_MUSIC		_T("Music")
#define FRIENDLY_NAME_VIDEOS	_T("Videos")
#define FRIENDLY_NAME_DOCUMENTS	_T("Documents")

/* See: http://msdn.microsoft.com/en-us/library/bb776902(v=VS.85).aspx#CFSTR_SHELLIDLIST */
#define HIDA_GetPIDLFolder(pida) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])
#define HIDA_GetPIDLItem(pida, i) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])

typedef struct
{
	WORD wLanguage;
	WORD wCodePage;
} LANGANDCODEPAGE;

typedef struct
{
	TCHAR szFileName[MAX_PATH];
} PastedFile_t;

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

/* Listview helper functions. */
int				ListView_SelectAllItems(HWND hListView);
int				ListView_InvertSelection(HWND hListView);
void			ListView_HandleInsertionMark(HWND hListView,int iItemFocus,POINT *ppt);
BOOL			ListView_SelectItem(HWND hListView,int nItem,BOOL bSelect);
BOOL			ListView_FocusItem(HWND hListView,int nItem,BOOL bFocus);
int				ListView_DeselectAllItems(HWND hListView);
DWORD			ListView_SetGridlines(HWND hListView,BOOL bEnableGridlines);
void			ListView_SetAutoArrange(HWND hListView,BOOL bAutoArrange);
HRESULT			ListView_SetBackgroundImage(HWND hListView,UINT Image);
void			ListView_SwapItems(HWND hListView,int iItem1,int iItem2);
void			ListView_SwapItemsNolParam(HWND hListView,int iItem1,int iItem2);
void			ListView_ActivateOneClickSelect(HWND hListView,BOOL bActivate,UINT HoverTime);

/* Tab control helper functions. */
void			TabCtrl_SwapItems(HWND hTabCtrl,int iItem1,int iItem2);
void			TabCtrl_SetItemText(HWND,int,TCHAR *);

/* Drive information. */
BOOL			GetDriveGeometryEx(TCHAR *lpszDrive,DISK_GEOMETRY_EX *pDiskGeometry,DWORD dwSize);
BOOL			GetDriveLayoutEx(TCHAR *lpszDrive,DRIVE_LAYOUT_INFORMATION_EX **pDriveLayout,DWORD BufSize);
BOOL			GetDrivePerformanceInfo(TCHAR *lpszDrive,DISK_PERFORMANCE *pDiskPerformance);
BOOL			GetFileSystemInfo(TCHAR *lpszDrive,FILESYSTEM_STATISTICS *pStatistics,DWORD BufSize);
BOOL			GetNtfsVolumeInfo(TCHAR *lpszDrive,NTFS_VOLUME_DATA_BUFFER *pNtfsVolumeInfo,DWORD BufSize);
TCHAR *			GetPartitionName(LARGE_INTEGER StartingOffset);
LARGE_INTEGER	GetDriveLength(TCHAR *lpszDrive);
BOOL			GetFileAllocationInfo(TCHAR *lpszFileName,STARTING_VCN_INPUT_BUFFER *pStartingVcn,RETRIEVAL_POINTERS_BUFFER *pRetrievalPointers,DWORD BufSize);
LONG			GetSectorSize(TCHAR *Drive);
LONG			GetClusterSize(TCHAR *Drive);
int				GetNumberOfUsedPartitions(DRIVE_LAYOUT_INFORMATION_EX *pDriveLayout);
LONG			GetFileSectorSize(TCHAR *FileName);
TCHAR			GetDriveNameFromMask(ULONG unitmask);

/* Drag and drop helpers. */
DWORD			DetermineCurrentDragEffect(DWORD grfKeyState,DWORD dwCurrentEffect,BOOL bDataAccept,BOOL bOnSameDrive);

/* File helpers. */
int				CreateFileTimeString(FILETIME *FileTime,TCHAR *Buffer,int MaxCharacters,BOOL bFriendlyDate);
BOOL			GetRealFileSize(TCHAR *FileName,PLARGE_INTEGER lpRealFileSize);
BOOL			CompareFileTypes(TCHAR *pszFile1,TCHAR *pszFile2);
BOOL			SetFileSparse(TCHAR *szFileName);
DWORD			BuildFileAttributeString(TCHAR *lpszFileName,TCHAR *Buffer,DWORD BufSize);
void			BuildFileAttributeStringInternal(DWORD dwFileAttributes,TCHAR *szOutput,DWORD cchMax);
size_t			GetFileOwner(TCHAR *szFile,TCHAR *szOwner,DWORD BufSize);
DWORD			GetNumFileHardLinks(TCHAR *lpszFileName);
int				ReadFileProperty(TCHAR *lpszFileName,DWORD dwPropertyId,TCHAR *lpszPropertyBuf,DWORD dwBufLen);
int				SetFileProperty(TCHAR *lpszFileName,DWORD dwPropertyType,TCHAR *szNewValue);
HRESULT			GetItemAttributes(TCHAR *szItemParsingPath,SFGAOF *pItemAttributes);
HRESULT			GetItemAttributes(LPITEMIDLIST pidl,SFGAOF *pItemAttributes);
BOOL			ExecuteFileAction(HWND hwnd,TCHAR *szVerb,TCHAR *szParameters,TCHAR *szStartDirectory,LPCITEMIDLIST pidl);
BOOL			GetVersionInfoString(TCHAR *szFileName,TCHAR *szVersionInfo,TCHAR *szBuffer,UINT cbBufLen);
BOOL			ReadImageProperty(TCHAR *lpszImage,UINT PropertyId,void *pPropBuffer,DWORD dwBufLen);
BOOL			IsImage(TCHAR *FileName);
WORD			GetFileLanguage(TCHAR *szFullFileName);
BOOL			GetFileProductVersion(TCHAR *szFullFileName,DWORD *pdwProductVersionLS,DWORD *pdwProductVersionMS);

/* Ownership and access. */
BOOL			GetProcessOwner(TCHAR *szOwner,DWORD BufSize);
BOOL			CheckGroupMembership(GroupType_t GroupType);

/* Menu helpers. */
BOOL			lCheckMenuItem(HMENU hMenu,UINT ItemID,BOOL bCheck);
BOOL			lEnableMenuItem(HMENU hMenu,UINT ItemID,BOOL bEnable);

/* Time helpers. */
BOOL			LocalSystemTimeToFileTime(LPSYSTEMTIME lpLocalTime,LPFILETIME lpFileTime);
BOOL			FileTimeToLocalSystemTime(LPFILETIME lpFileTime,LPSYSTEMTIME lpLocalTime);

/* File streams. */
int				DumpSummaryInformationStream(TCHAR *lpszInputFile,TCHAR *lpszOutputFile);
int				EnumFileStreams(TCHAR *lpszFileName);
void			ReadFileStreams(TCHAR *lpszFile);

/* Bitmap helper functions. */
HRESULT			GetBitmapDimensions(TCHAR *FileName,SIZE *BitmapSize);

/* File slack. */
void			WriteFileSlack(TCHAR *szFileName,void *pData,int iDataSize);
int				ReadFileSlack(TCHAR *FileName,TCHAR *pszSlack,int iBufferLen);

/* Default icon indices. */
int				GetDefaultIcon(int iIconType);
int				GetDefaultFolderIconIndex(void);
int				GetDefaultFileIconIndex(void);

/* Infotips. */
HRESULT			GetFileInfoTip(HWND hwnd,LPCITEMIDLIST pidlDirectory,LPCITEMIDLIST *pridl,TCHAR *szInfoTip,UINT cchMax);

/* User interaction. */
BOOL			GetFileNameFromUser(HWND hwnd,TCHAR *FullFileName,TCHAR *InitialDirectory);

/* Device related. */
BOOL			GetMonitorDeviceName(TCHAR *lpszMonitorName,DWORD BufSize);
TCHAR			*DecodePrinterStatus(DWORD dwStatus);
void			RetrieveAdapterInfo(void);

/* Process helpers. */
DWORD			GetCurrentProcessImageName(TCHAR *szImageName,DWORD nSize);

/* Computer information. */
void			GetCPUBrandString(char *pszCPUBrand,UINT cchBuf);

/* Media metadata. */
HRESULT			GetMediaMetadata(TCHAR *szFileName,LPCWSTR szAttribute,BYTE **pszOutput);

/* General helper functions. */
void			FormatSizeString(ULARGE_INTEGER lFileSize,TCHAR *pszFileSize,size_t cchBuf);
void			FormatSizeString(ULARGE_INTEGER lFileSize,TCHAR *pszFileSize,size_t cchBuf,BOOL bForceSize,SizeDisplayFormat_t sdf);
HINSTANCE		StartCommandPrompt(TCHAR *Directory);
BOOL			SetProcessTokenPrivilege(DWORD ProcessId,TCHAR *PrivilegeName,BOOL bEnablePrivilege);
void			ShowLastError(void);
int				WriteTextToRichEdit(HWND hRichEdit,TCHAR *fmt,...);
int				GetRectHeight(RECT *rc);
int				GetRectWidth(RECT *rc);
HANDLE			CopyTextToClipboard(TCHAR *szTextData);
void			CheckItem(HWND hwnd,BOOL bCheck);
BOOL			IsItemChecked(HWND hwnd);
BOOL			lShowWindow(HWND hwnd,BOOL bShowWindow);
void			EnumLoggedOnUsers(void);
TCHAR			*PrintComma(unsigned long nPrint);
TCHAR			*PrintCommaLargeNum(LARGE_INTEGER lPrint);
BOOL			CheckWildcardMatch(const TCHAR *szWildcard,const TCHAR *szString,BOOL bCaseSensitive);
BOOL			CheckWildcardMatchInternal(const TCHAR *szWildcard,const TCHAR *szString,BOOL bCaseSensitive);
void			ReplaceCharacters(TCHAR *str,char ch,char replacement);
TCHAR			*GetToken(TCHAR *ptr,TCHAR *Buffer,TCHAR *BufferLength);
HRESULT			DecodeFriendlyPath(TCHAR *szFriendlyPath,TCHAR *szParsingPath);
void			AddGripperStyle(UINT *fStyle,BOOL bAddGripper);
void			AddWindowStyle(HWND hwnd,UINT fStyle,BOOL bAdd);
void			ReplaceCharacterWithString(TCHAR *szBaseString,TCHAR *szOutput,UINT cchMax,TCHAR chToReplace,TCHAR *szReplacement);
void			CenterWindow(HWND hParent,HWND hChild);
TCHAR			*ReplaceSubString(TCHAR *szString,TCHAR *szSubString,TCHAR *szReplacement);
void			UpdateToolbarBandSizing(HWND hRebar,HWND hToolbar);
void			MergeDateTime(SYSTEMTIME *pstOutput,SYSTEMTIME *pstDate,SYSTEMTIME *pstTime);
void			GetWindowString(HWND hwnd,std::wstring &str);

#endif