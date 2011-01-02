#ifndef WILDCARDSELECT_INCLUDED
#define WILDCARDSELECT_INCLUDED

#include <list>
#include <string>
#include "../Helper/BaseDialog.h"

#import <msxml3.dll> raw_interfaces_only

class CWildcardSelectDialog;

class CWildcardSelectDialogPersistentSettings
{
public:

	~CWildcardSelectDialogPersistentSettings();

	static CWildcardSelectDialogPersistentSettings &GetInstance();

	/* Registry save/load settings. */
	void	SaveSettings(HKEY hParentKey);
	void	LoadSettings(HKEY hParentKey);

	/* XML save/load settings. */
	void	SaveSettings(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pe);
	void	LoadSettings(MSXML2::IXMLDOMNamedNodeMap *pam,long lChildNodes);

private:

	friend CWildcardSelectDialog;

	static const TCHAR REGISTRY_SETTINGS_KEY[];

	CWildcardSelectDialogPersistentSettings();

	CWildcardSelectDialogPersistentSettings(const CWildcardSelectDialogPersistentSettings &);
	CWildcardSelectDialogPersistentSettings & operator=(const CWildcardSelectDialogPersistentSettings &);

	TCHAR			m_szPattern[256];
	std::list<std::wstring>	m_PatternList;

	BOOL			m_bStateSaved;
	POINT			m_ptDialog;
};

class CWildcardSelectDialog : public CBaseDialog
{
public:

	CWildcardSelectDialog(HINSTANCE hInstance,int iResource,HWND hParent,BOOL bSelect);
	~CWildcardSelectDialog();

protected:

	BOOL	OnInitDialog();
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnClose();
	BOOL	OnDestroy();

private:

	void	OnOk();
	void	OnCancel();

	void	SaveState();

	CWildcardSelectDialogPersistentSettings	*m_pwsdps;

	BOOL	m_bSelect;
};

#endif