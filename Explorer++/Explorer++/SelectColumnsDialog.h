#ifndef SELECTCOLUMNSDIALOG_INCLUDED
#define SELECTCOLUMNSDIALOG_INCLUDED

#include "../Helper/BaseDialog.h"
#include "../Helper/ResizableDialog.h"
#include "../Helper/DialogSettings.h"

class CSelectColumnsDialog;

class CSelectColumnsDialogPersistentSettings : public CDialogSettings
{
public:

	~CSelectColumnsDialogPersistentSettings();

	static CSelectColumnsDialogPersistentSettings &GetInstance();

private:

	friend CSelectColumnsDialog;

	static const TCHAR SETTINGS_KEY[];

	CSelectColumnsDialogPersistentSettings();

	CSelectColumnsDialogPersistentSettings(const CSelectColumnsDialogPersistentSettings &);
	CSelectColumnsDialogPersistentSettings & operator=(const CSelectColumnsDialogPersistentSettings &);
};

class CSelectColumnsDialog : public CBaseDialog
{
public:

	CSelectColumnsDialog(HINSTANCE hInstance,int iResource,HWND hParent);
	~CSelectColumnsDialog();

protected:

	BOOL	OnInitDialog();
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnNotify(NMHDR *pnmhdr);
	BOOL	OnClose();

	void	SaveState();

	void	GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc,std::list<CResizableDialog::Control_t> &ControlList);

private:

	void	OnOk();
	void	OnCancel();
	void	OnLvnItemChanging(NMLISTVIEW *pnmlv);
	void	OnMoveColumn(bool bUp);

	BOOL	m_bColumnsSwapped;

	CSelectColumnsDialogPersistentSettings	*m_pscdps;
};

#endif