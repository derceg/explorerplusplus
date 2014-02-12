#pragma once

#include <list>
#include <string>
#include "Macros.h"

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

	bool			GetSettingsKey(TCHAR *out, size_t cchMax) const;

protected:

	void			SaveDialogPosition(HWND hDlg);
	void			RestoreDialogPosition(HWND hDlg,bool bRestoreSize);

	BOOL			m_bStateSaved;

private:

	DISALLOW_COPY_AND_ASSIGN(CDialogSettings);

	static const TCHAR SETTING_POSITION[];
	static const TCHAR SETTING_POSITION_X[];
	static const TCHAR SETTING_POSITION_Y[];
	static const TCHAR SETTING_WIDTH[];
	static const TCHAR SETTING_HEIGHT[];

	virtual void	SaveExtraRegistrySettings(HKEY hKey);
	virtual void	LoadExtraRegistrySettings(HKEY hKey);

	virtual void	SaveExtraXMLSettings(MSXML2::IXMLDOMDocument *pXMLDom, MSXML2::IXMLDOMElement *pParentNode);
	virtual void	LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue);

	const std::wstring m_szSettingsKey;
	const bool		m_bSavePosition;

	POINT			m_ptDialog;
	int				m_iWidth;
	int				m_iHeight;
};