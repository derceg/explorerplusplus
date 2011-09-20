#ifndef BASEDIALOG_INCLUDED
#define BASEDIALOG_INCLUDED

#include "ResizableDialog.h"
#include "MessageForwarder.h"
#include "ReferenceCount.h"

__interface IModelessDialogNotification : public IReferenceCount
{
	void OnModelessDialogDestroy(int iResource);
};

/* Provides a degree of abstraction off a standard dialog.
For instance, provides the ability for a class to manage
a dialog without having to handle the dialog procedure
directly. */
class CBaseDialog : public CMessageForwarder
{
	friend INT_PTR CALLBACK BaseDialogProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:

	enum DialogSizeConstraint
	{
		DIALOG_SIZE_CONSTRAINT_NONE,
		DIALOG_SIZE_CONSTRAINT_X,
		DIALOG_SIZE_CONSTRAINT_Y
	};

	CBaseDialog(HINSTANCE hInstance,int iResource,HWND hParent,bool bResizable);
	~CBaseDialog();

	INT_PTR			ShowModalDialog();
	HWND			ShowModelessDialog(IModelessDialogNotification *pmdn = NULL);

protected:

	HINSTANCE		GetInstance() const;

	INT_PTR			GetDefaultReturnValue(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

	virtual void	GetResizableControlInformation(DialogSizeConstraint &dsc,std::list<CResizableDialog::Control_t> &ControlList);
	virtual void	SaveState();

	HWND			m_hDlg;

private:

	INT_PTR CALLBACK	BaseDialogProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

	HINSTANCE		m_hInstance;
	int				m_iResource;
	HWND			m_hParent;
	IModelessDialogNotification	*m_pmdn;

	BOOL			m_bShowingModelessDialog;

	/* Used only with resizable dialogs. */
	bool			m_bResizable;
	DialogSizeConstraint	m_dsc;
	int				m_iMinWidth;
	int				m_iMinHeight;
	CResizableDialog	*m_prd;
};

#endif