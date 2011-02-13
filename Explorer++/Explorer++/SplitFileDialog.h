#ifndef SPLITFILEDIALOG_INCLUDED
#define SPLITFILEDIALOG_INCLUDED

#include <list>
#include <string>
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ReferenceCount.h"

class CSplitFileDialog;

class CSplitFileDialogPersistentSettings : public CDialogSettings
{
public:

	~CSplitFileDialogPersistentSettings();

	static CSplitFileDialogPersistentSettings &GetInstance();

protected:

	void			SaveExtraRegistrySettings(HKEY hKey);
	void			LoadExtraRegistrySettings(HKEY hKey);

	void			SaveExtraXMLSettings(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pParentNode);
	void			LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue);

private:

	friend CSplitFileDialog;

	static const TCHAR SETTINGS_KEY[];

	CSplitFileDialogPersistentSettings();

	CSplitFileDialogPersistentSettings(const CSplitFileDialogPersistentSettings &);
	CSplitFileDialogPersistentSettings & operator=(const CSplitFileDialogPersistentSettings &);

	std::wstring	m_strSplitSize;
	std::wstring	m_strSplitGroup;
};

class CSplitFile : public CReferenceCount
{
public:
	
	CSplitFile(HWND hDlg,std::wstring strFullFilename,std::wstring strOutputFilename,std::wstring strOutputDirectory,UINT uSplitSize);
	~CSplitFile();

	void	SplitFile();
	void	StopSplitting();

private:

	void				SplitFileInternal(HANDLE hInputFile,const LARGE_INTEGER &lFileSize);
	void				ProcessFilename(int nSplitsMade,std::wstring &strOutputFullFilename);

	HWND				m_hDlg;

	std::wstring		m_strFullFilename;
	std::wstring		m_strOutputFilename;
	std::wstring		m_strOutputDirectory;
	UINT				m_uSplitSize;

	CRITICAL_SECTION	m_csStop;
	bool				m_bStopSplitting;
};

class CSplitFileDialog : public CBaseDialog
{
public:

	CSplitFileDialog(HINSTANCE hInstance,int iResource,HWND hParent,std::wstring strFullFilename);
	~CSplitFileDialog();

protected:

	BOOL	OnInitDialog();
	BOOL	OnTimer(int iTimerID);
	INT_PTR	OnCtlColorStatic(HWND hwnd,HDC hdc);
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnClose();
	BOOL	OnDestroy();

	void	SaveState();

	void	OnPrivateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam);

private:

	enum SizeType_t
	{
		SIZE_TYPE_BYTES,
		SIZE_TYPE_KB,
		SIZE_TYPE_MB,
		SIZE_TYPE_GB,
	};

	enum ErrorType_t
	{
		ERROR_NONE,
		ERROR_OUTPUT_FILENAME_EMPTY,
		ERROR_OUTPUT_FILENAME_CONSTANT,
		ERROR_OUTPUT_DIRECTORY_EMPTY,
		ERROR_SPLIT_SIZE
	};

	static const COLORREF HELPER_TEXT_COLOR = RGB(120,120,120);

	static const int KB = (1024);
	static const int MB = (1024 * 1024);
	static const int GB = (1024 * 1024 * 1024);

	static const UINT_PTR ELPASED_TIMER_ID = 1;
	static const UINT_PTR ELPASED_TIMER_TIMEOUT = 1000;

	void	OnOk();
	void	OnCancel();
	void	OnChangeOutputDirectory();
	void	OnSplitFinished();

	std::wstring	m_strFullFilename;
	bool			m_bSplittingFile;
	bool			m_bStopSplitting;

	std::tr1::unordered_map<int,SizeType_t>	m_SizeMap;

	CSplitFile		*m_pSplitFile;
	HFONT			m_hHelperTextFont;

	TCHAR			m_szOk[32];
	UINT			m_uElapsedTime;

	ErrorType_t		m_CurrentError;

	CSplitFileDialogPersistentSettings	*m_psfdps;
};

#endif