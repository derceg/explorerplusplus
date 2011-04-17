#ifndef SETDEFAULTCOLUMNSDIALOG_INCLUDED
#define SETDEFAULTCOLUMNSDIALOG_INCLUDED

#include <unordered_map>
#include "../Helper/BaseDialog.h"
#include "../Helper/ResizableDialog.h"
#include "../Helper/DialogSettings.h"

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

	std::wstring	m_strFolder;
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

	enum FolderType_t
	{
		FOLDER_TYPE_GENERAL,
		FOLDER_TYPE_COMPUTER,
		FOLDER_TYPE_CONTROL_PANEL,
		FOLDER_TYPE_NETWORK,
		FOLDER_TYPE_NETWORK_PLACES,
		FOLDER_TYPE_PRINTERS,
		FOLDER_TYPE_RECYCLE_BIN
	};

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

	std::tr1::unordered_map<int,FolderType_t>	m_FolderMap;

	CSetDefaultColumnsDialogPersistentSettings	*m_psdcdps;
};


#endif