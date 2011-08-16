#ifndef MERGEFILESDIALOG_INCLUDED
#define MERGEFILESDIALOG_INCLUDED

#include "../Helper/BaseDialog.h"
#include "../Helper/ResizableDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ReferenceCount.h"

class CMergeFilesDialog;

class CMergeFilesDialogPersistentSettings : public CDialogSettings
{
public:

	~CMergeFilesDialogPersistentSettings();

	static CMergeFilesDialogPersistentSettings &GetInstance();

private:

	friend CMergeFilesDialog;

	static const TCHAR SETTINGS_KEY[];

	CMergeFilesDialogPersistentSettings();

	CMergeFilesDialogPersistentSettings(const CMergeFilesDialogPersistentSettings &);
	CMergeFilesDialogPersistentSettings & operator=(const CMergeFilesDialogPersistentSettings &);
};

class CMergeFiles : public CReferenceCount
{
public:
	
	CMergeFiles(HWND hDlg,std::wstring strOutputFilename,std::list<std::wstring> FullFilenameList);
	~CMergeFiles();

	void					StartMerging();
	void					StopMerging();

private:

	HWND					m_hDlg;

	std::wstring			m_strOutputFilename;
	std::list<std::wstring>	m_FullFilenameList;

	CRITICAL_SECTION		m_csStop;
	bool					m_bstopMerging;
};

class CMergeFilesDialog : public CBaseDialog
{
public:

	CMergeFilesDialog(HINSTANCE hInstance,int iResource,HWND hParent,std::wstring strOutputDirectory,std::list<std::wstring> FullFilenameList,BOOL bShowFriendlyDates);
	~CMergeFilesDialog();

protected:

	BOOL	OnInitDialog();
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnClose();
	BOOL	OnDestroy();

	void	SaveState();

	void	OnPrivateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam);

	void	GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc,std::list<CResizableDialog::Control_t> &ControlList);

private:

	void	OnOk();
	void	OnCancel();
	void	OnChangeOutputDirectory();
	void	OnMove(bool bUp);
	void	OnFinished();

	std::wstring			m_strOutputDirectory;
	std::list<std::wstring>	m_FullFilenameList;
	BOOL					m_bShowFriendlyDates;

	CMergeFiles				*m_pMergeFiles;
	bool					m_bMergingFiles;
	bool					m_bStopMerging;
	TCHAR					m_szOk[32];

	HICON					m_hDialogIcon;

	CMergeFilesDialogPersistentSettings	*m_pmfdps;
};

#endif