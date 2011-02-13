#ifndef DIALOGSETTINGS_INCLUDED
#define DIALOGSETTINGS_INCLUDED

#include <list>
#include <string>

#import <msxml3.dll> raw_interfaces_only

class CDialogSettings
{
public:

	CDialogSettings(const TCHAR *szSettingsKey,bool bSavePosition = true);
	virtual ~CDialogSettings();

	void			SaveRegistrySettings(HKEY hParentKey);
	void			LoadRegistrySettings(HKEY hParentKey);

	void			SaveXMLSettings(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pe);
	void			LoadXMLSettings(MSXML2::IXMLDOMNamedNodeMap *pam,long lChildNodes);

protected:

	virtual void	SaveExtraRegistrySettings(HKEY hKey);
	virtual void	LoadExtraRegistrySettings(HKEY hKey);

	virtual void	SaveExtraXMLSettings(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pParentNode);
	virtual void	LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue);

	void			SaveDialogPosition(HWND hDlg);
	void			RestoreDialogPosition(HWND hDlg,bool bRestoreSize);

	BOOL			m_bStateSaved;
	POINT			m_ptDialog;
	int				m_iWidth;
	int				m_iHeight;

private:

	TCHAR			m_szSettingsKey[256];

	bool			m_bSavePosition;
};

#endif