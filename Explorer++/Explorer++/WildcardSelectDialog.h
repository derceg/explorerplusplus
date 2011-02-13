#ifndef WILDCARDSELECT_INCLUDED
#define WILDCARDSELECT_INCLUDED

#include <list>
#include <string>
#include "../Helper/BaseDialog.h"
#include "../Helper/ResizableDialog.h"
#include "../Helper/DialogSettings.h"

#import <msxml3.dll> raw_interfaces_only

class CWildcardSelectDialog;

class CWildcardSelectDialogPersistentSettings : public CDialogSettings
{
public:

	~CWildcardSelectDialogPersistentSettings();

	static			CWildcardSelectDialogPersistentSettings &GetInstance();

protected:

	void			SaveExtraRegistrySettings(HKEY hKey);
	void			LoadExtraRegistrySettings(HKEY hKey);

	void			SaveExtraXMLSettings(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pParentNode);
	void			LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue);

private:

	friend			CWildcardSelectDialog;

	static const	TCHAR SETTINGS_KEY[];

	CWildcardSelectDialogPersistentSettings();

	CWildcardSelectDialogPersistentSettings(const CWildcardSelectDialogPersistentSettings &);
	CWildcardSelectDialogPersistentSettings & operator=(const CWildcardSelectDialogPersistentSettings &);

	TCHAR			m_szPattern[256];
	std::list<std::wstring>	m_PatternList;
};

class CWildcardSelectDialog : public CBaseDialog
{
public:

	CWildcardSelectDialog(HINSTANCE hInstance,int iResource,HWND hParent,BOOL bSelect,IExplorerplusplus *pexpp);
	~CWildcardSelectDialog();

protected:

	BOOL	OnInitDialog();
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnClose();

	void	SaveState();

	void	GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc,std::list<CResizableDialog::Control_t> &ControlList);

private:

	void				OnOk();
	void				OnCancel();
	void				SelectItems(TCHAR *szPattern);

	IExplorerplusplus	*m_pexpp;
	BOOL				m_bSelect;

	CWildcardSelectDialogPersistentSettings	*m_pwsdps;
};

#endif