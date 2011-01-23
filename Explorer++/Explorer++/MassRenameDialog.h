#ifndef MASSRENAMEDIALOG_INCLUDED
#define MASSRENAMEDIALOG_INCLUDED

#include "../Helper/BaseDialog.h"
#include "../Helper/ResizableDialog.h"
#include "../Helper/DialogSettings.h"

class CMassRenameDialog;

class CMassRenameDialogPersistentSettings : public CDialogSettings
{
public:

	~CMassRenameDialogPersistentSettings();

	static CMassRenameDialogPersistentSettings &GetInstance();

private:

	friend CMassRenameDialog;

	static const TCHAR SETTINGS_KEY[];

	CMassRenameDialogPersistentSettings();

	CMassRenameDialogPersistentSettings(const CMassRenameDialogPersistentSettings &);
	CMassRenameDialogPersistentSettings & operator=(const CMassRenameDialogPersistentSettings &);
};

class CMassRenameDialog : public CBaseDialog
{
public:

	CMassRenameDialog(HINSTANCE hInstance,int iResource,HWND hParent,std::list<std::wstring> FullFilenameList);
	~CMassRenameDialog();

protected:

	BOOL	OnInitDialog();
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnGetMinMaxInfo(LPMINMAXINFO pmmi);
	BOOL	OnSize(int iType,int iWidth,int iHeight);
	BOOL	OnClose();
	BOOL	OnDestroy();

private:

	void	OnOk();
	void	OnCancel();

	void	SaveState();

	void	InitializeControlStates();
	void	ProcessFileName(const std::wstring strTarget,const std::wstring strFilename,int iFileIndex,std::wstring &strOutput);

	std::list<std::wstring>	m_FullFilenameList;
	HWND	m_hGripper;
	HICON	m_hDialogIcon;
	HICON	m_hMoreIcon;

	CResizableDialog	*m_prd;

	int		m_iMinWidth;
	int		m_iMinHeight;

	int		m_iListViewWidthDelta;
	int		m_iListViewHeightDelta;

	CMassRenameDialogPersistentSettings	*m_pmrdps;
};

#endif