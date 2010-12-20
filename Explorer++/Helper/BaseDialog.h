#ifndef BASEDIALOG_INCLUDED
#define BASEDIALOG_INCLUDED

__interface IModelessDialogNotification
{
public:

	virtual void	OnModelessDialogDestroy();
};

/* Provides a degree of abstraction off a standard dialog.
For instance, provides the ability for a class to manage
a dialog without having to handle the dialog procedure
directly. */
class CBaseDialog
{
public:

	CBaseDialog(HINSTANCE hInstance,int iResource,HWND hParent);
	~CBaseDialog();

	INT_PTR CALLBACK	BaseDialogProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

	INT_PTR	ShowModalDialog();
	HWND	ShowModelessDialog();

protected:

	virtual BOOL	OnInitDialog();
	virtual BOOL	OnTimer(int iTimerID);
	virtual BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	virtual BOOL	OnNotify(NMHDR *pnmhdr);
	virtual BOOL	OnGetMinMaxInfo(LPMINMAXINFO pmmi);
	virtual BOOL	OnSize(int iType,int iWidth,int iHeight);
	virtual BOOL	OnClose();
	virtual BOOL	OnDestroy();
	virtual BOOL	OnNcDestroy();

	/* For private application messages in
	the range WM_APP (0x8000) - 0xBFFF. */
	virtual void	OnPrivateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam);

	HWND		m_hDlg;

protected:

	HINSTANCE	GetInstance();

private:

	HINSTANCE	m_hInstance;
	int			m_iResource;
	HWND		m_hParent;

	BOOL		m_bShowingModelessDialog;
};

#endif