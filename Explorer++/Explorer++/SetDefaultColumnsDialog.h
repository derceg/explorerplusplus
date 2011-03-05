#ifndef SETDEFAULTCOLUMNSDIALOG_INCLUDED
#define SETDEFAULTCOLUMNSDIALOG_INCLUDED

#include "../Helper/BaseDialog.h"
#include "../Helper/ResizableDialog.h"
#include "../Helper/DialogSettings.h"

class CSetDefaultColumnsDialog;

class CSetDefaultColumnsDialogPersistentSettings : public CDialogSettings
{
public:

	~CSetDefaultColumnsDialogPersistentSettings();

	static CSetDefaultColumnsDialogPersistentSettings &GetInstance();

private:

	friend CSetDefaultColumnsDialog;

	static const TCHAR SETTINGS_KEY[];

	CSetDefaultColumnsDialogPersistentSettings();

	CSetDefaultColumnsDialogPersistentSettings(const CSetDefaultColumnsDialogPersistentSettings &);
	CSetDefaultColumnsDialogPersistentSettings & operator=(const CSetDefaultColumnsDialogPersistentSettings &);
};

class CSetDefaultColumnsDialog : public CBaseDialog
{
public:

	CSetDefaultColumnsDialog(HINSTANCE hInstance,int iResource,HWND hParent);
	~CSetDefaultColumnsDialog();

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
	void	OnCbnSelChange();
	void	OnLvnItemChanging(NMLISTVIEW *pnmlv);
	void	OnMoveColumn(bool bUp);

	std::list<Column_t>	m_RealFolderColumnList;
	std::list<Column_t>	m_MyComputerColumnList;
	std::list<Column_t>	m_ControlPanelColumnList;
	std::list<Column_t>	m_RecycleBinColumnList;
	std::list<Column_t>	m_PrintersColumnList;
	std::list<Column_t>	m_NetworkConnectionsColumnList;
	std::list<Column_t>	m_MyNetworkPlacesColumnList;

	/* Index of folder types within the combo box. */
	int					m_iControlPanel;
	int					m_iGeneral;
	int					m_iMyComputer;
	int					m_iNetwork;
	int					m_iNetworkPlaces;
	int					m_iPrinters;
	int					m_iRecycleBin;
	int					m_iPreviousTypeSel;

	CSetDefaultColumnsDialogPersistentSettings	*m_psdcdps;
};


#endif