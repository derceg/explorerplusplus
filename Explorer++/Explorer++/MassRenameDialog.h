#ifndef MASSRENAMEDIALOG_INCLUDED
#define MASSRENAMEDIALOG_INCLUDED

#include "../Helper/BaseDialog.h"
#include "../Helper/ResizableDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/FileActionHandler.h"

class CMassRenameDialog;

class CMassRenameDialogPersistentSettings : public CDialogSettings
{
public:

	~CMassRenameDialogPersistentSettings();

	static CMassRenameDialogPersistentSettings &GetInstance();

protected:

	void			SaveExtraRegistrySettings(HKEY hKey);
	void			LoadExtraRegistrySettings(HKEY hKey);

	void			SaveExtraXMLSettings(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pParentNode);
	void			LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue);

private:

	friend CMassRenameDialog;

	static const TCHAR SETTINGS_KEY[];
	static const int DEFAULT_MASS_RENAME_COLUMN_WIDTH = 250;

	CMassRenameDialogPersistentSettings();

	CMassRenameDialogPersistentSettings(const CMassRenameDialogPersistentSettings &);
	CMassRenameDialogPersistentSettings & operator=(const CMassRenameDialogPersistentSettings &);

	int	m_iColumnWidth1;
	int	m_iColumnWidth2;
};

class CMassRenameDialog : public CBaseDialog
{
public:

	CMassRenameDialog(HINSTANCE hInstance,int iResource,HWND hParent,std::list<std::wstring> FullFilenameList,CFileActionHandler *pFileActionHandler);
	~CMassRenameDialog();

protected:

	BOOL	OnInitDialog();
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnClose();
	BOOL	OnDestroy();

	void	SaveState();

	void	GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc,std::list<CResizableDialog::Control_t> &ControlList);

private:

	void	OnOk();
	void	OnCancel();

	void	ProcessFileName(const std::wstring strTarget,const std::wstring strFilename,int iFileIndex,std::wstring &strOutput);

	std::list<std::wstring>	m_FullFilenameList;
	HICON					m_hDialogIcon;
	HICON					m_hMoreIcon;
	CFileActionHandler		*m_pFileActionHandler;

	CMassRenameDialogPersistentSettings	*m_pmrdps;
};

#endif