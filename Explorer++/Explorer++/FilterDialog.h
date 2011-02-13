#ifndef FILTERDIALOG_INCLUDED
#define FILTERDIALOG_INCLUDED

#include "../Helper/BaseDialog.h"
#include "../Helper/ResizableDialog.h"
#include "../Helper/DialogSettings.h"

#import <msxml3.dll> raw_interfaces_only

class CFilterDialog;

class CFilterDialogPersistentSettings : public CDialogSettings
{
public:

	~CFilterDialogPersistentSettings();

	static CFilterDialogPersistentSettings &GetInstance();

protected:

	void			SaveExtraRegistrySettings(HKEY hKey);
	void			LoadExtraRegistrySettings(HKEY hKey);

	void			SaveExtraXMLSettings(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pParentNode);
	void			LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue);

private:

	friend CFilterDialog;

	static const TCHAR SETTINGS_KEY[];

	CFilterDialogPersistentSettings();

	CFilterDialogPersistentSettings(const CFilterDialogPersistentSettings &);
	CFilterDialogPersistentSettings & operator=(const CFilterDialogPersistentSettings &);

	std::list<std::wstring>	m_FilterList;
};

class CFilterDialog : public CBaseDialog
{
public:

	CFilterDialog(HINSTANCE hInstance,int iResource,HWND hParent,IExplorerplusplus *pexpp);
	~CFilterDialog();

protected:

	BOOL				OnInitDialog();
	BOOL				OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL				OnClose();
	BOOL				OnDestroy();
	void				SaveState();

	void				GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc,std::list<CResizableDialog::Control_t> &ControlList);

private:

	void				OnOk();
	void				OnCancel();

	IExplorerplusplus	*m_pexpp;
	HICON				m_hDialogIcon;

	CFilterDialogPersistentSettings	*m_pfdps;
};

#endif