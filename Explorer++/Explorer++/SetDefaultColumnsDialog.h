#ifndef SETDEFAULTCOLUMNSDIALOG_INCLUDED
#define SETDEFAULTCOLUMNSDIALOG_INCLUDED

#include <unordered_map>
#include "../Helper/BaseDialog.h"
#include "../Helper/ResizableDialog.h"
#include "../Helper/DialogSettings.h"

enum FolderType_t
{
	FOLDER_TYPE_GENERAL = 0,
	FOLDER_TYPE_COMPUTER = 1,
	FOLDER_TYPE_CONTROL_PANEL = 2,
	FOLDER_TYPE_NETWORK = 3,
	FOLDER_TYPE_NETWORK_PLACES = 4,
	FOLDER_TYPE_PRINTERS = 5,
	FOLDER_TYPE_RECYCLE_BIN = 6
};

class CSetDefaultColumnsDialog;

class CSetDefaultColumnsDialogPersistentSettings : public CDialogSettings
{
public:

	~CSetDefaultColumnsDialogPersistentSettings();

	static CSetDefaultColumnsDialogPersistentSettings &GetInstance();

protected:

	void			SaveExtraRegistrySettings(HKEY hKey);
	void			LoadExtraRegistrySettings(HKEY hKey);

	void			SaveExtraXMLSettings(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pParentNode);
	void			LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue);

private:

	friend CSetDefaultColumnsDialog;

	static const TCHAR SETTINGS_KEY[];

	CSetDefaultColumnsDialogPersistentSettings();

	CSetDefaultColumnsDialogPersistentSettings(const CSetDefaultColumnsDialogPersistentSettings &);
	CSetDefaultColumnsDialogPersistentSettings & operator=(const CSetDefaultColumnsDialogPersistentSettings &);

	FolderType_t	m_FolderType;
};

class CSetDefaultColumnsDialog : public CBaseDialog
{
public:

	CSetDefaultColumnsDialog(HINSTANCE hInstance,int iResource,HWND hParent,IExplorerplusplus *pexpp,std::list<Column_t> *pRealFolderColumnList,std::list<Column_t> *pMyComputerColumnList,
		std::list<Column_t> *pControlPanelColumnList,std::list<Column_t> *pRecycleBinColumnList,std::list<Column_t> *pPrintersColumnList,std::list<Column_t> *pNetworkConnectionsColumnList,
		std::list<Column_t> *pMyNetworkPlacesColumnList);
	~CSetDefaultColumnsDialog();

protected:

	BOOL	OnInitDialog();
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnNotify(NMHDR *pnmhdr);
	BOOL	OnClose();
	BOOL	OnDestroy();

	void	SaveState();

	void	GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc,std::list<CResizableDialog::Control_t> &ControlList);

private:

	void	OnOk();
	void	OnCancel();
	void	OnCbnSelChange();
	void	OnLvnItemChanging(NMLISTVIEW *pnmlv);
	void	OnMoveColumn(bool bUp);

	void	SaveCurrentColumnState(FolderType_t FolderType);
	void	SetupFolderColumns(FolderType_t FolderType);

	std::list<Column_t>	*GetCurrentColumnList(FolderType_t FolderType);

	IExplorerplusplus	*m_pexpp;

	std::list<Column_t>	*m_pRealFolderColumnList;
	std::list<Column_t>	*m_pMyComputerColumnList;
	std::list<Column_t>	*m_pControlPanelColumnList;
	std::list<Column_t>	*m_pRecycleBinColumnList;
	std::list<Column_t>	*m_pPrintersColumnList;
	std::list<Column_t>	*m_pNetworkConnectionsColumnList;
	std::list<Column_t>	*m_pMyNetworkPlacesColumnList;

	std::tr1::unordered_map<int,FolderType_t>	m_FolderMap;
	FolderType_t		m_PreviousFolderType;

	HICON				m_hDialogIcon;

	CSetDefaultColumnsDialogPersistentSettings	*m_psdcdps;
};


#endif