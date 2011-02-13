#ifndef DESTROYFILESDIALOG_INCLUDED
#define DESTROYFILESDIALOG_INCLUDED

#include "../Helper/BaseDialog.h"
#include "../Helper/ResizableDialog.h"
#include "../Helper/DialogSettings.h"

class CDestroyFilesDialog;

class CDestroyFilesDialogPersistentSettings : public CDialogSettings
{
public:

	~CDestroyFilesDialogPersistentSettings();

	static CDestroyFilesDialogPersistentSettings &GetInstance();

protected:

	void			SaveExtraRegistrySettings(HKEY hKey);
	void			LoadExtraRegistrySettings(HKEY hKey);

	void			SaveExtraXMLSettings(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pParentNode);
	void			LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue);

private:

	friend CDestroyFilesDialog;

	static const TCHAR SETTINGS_KEY[];

	CDestroyFilesDialogPersistentSettings();

	CDestroyFilesDialogPersistentSettings(const CDestroyFilesDialogPersistentSettings &);
	CDestroyFilesDialogPersistentSettings & operator=(const CDestroyFilesDialogPersistentSettings &);

	UINT	m_uOverwriteMethod;
};

class CDestroyFilesDialog : public CBaseDialog
{
public:

	CDestroyFilesDialog(HINSTANCE hInstance,int iResource,HWND hParent,std::list<std::wstring> FullFilenameList);
	~CDestroyFilesDialog();

protected:

	BOOL	OnInitDialog();
	INT_PTR	OnCtlColorStatic(HWND hwnd,HDC hdc);
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnClose();

	void	SaveState();

	void	GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc,std::list<CResizableDialog::Control_t> &ControlList);

private:

	void	OnOk();
	void	OnCancel();
	void	OnConfirmDestroy();

	std::list<std::wstring>	m_FullFilenameList;

	CDestroyFilesDialogPersistentSettings	*m_pdfdps;
};

#endif