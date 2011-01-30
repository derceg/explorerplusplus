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
	BOOL	OnGetMinMaxInfo(LPMINMAXINFO pmmi);
	BOOL	OnSize(int iType,int iWidth,int iHeight);
	BOOL	OnClose();
	BOOL	OnDestroy();

private:

	void				OnOk();
	void				OnCancel();
	void				SelectItems(TCHAR *szPattern);

	void				SaveState();

	void				InitializeControlStates();

	IExplorerplusplus	*m_pexpp;
	BOOL				m_bSelect;

	HWND				m_hGripper;

	int					m_iMinWidth;
	int					m_iMinHeight;
	CResizableDialog	*m_prd;

	CWildcardSelectDialogPersistentSettings	*m_pwsdps;
};

#endif