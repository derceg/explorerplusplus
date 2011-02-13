#ifndef COLORRULEDIALOG_INCLUDED
#define COLORRULEDIALOG_INCLUDED

#include "CustomizeColorsDialog.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"

class CColorRuleDialog;

class CColorRuleDialogPersistentSettings : public CDialogSettings
{
public:

	~CColorRuleDialogPersistentSettings();

	static CColorRuleDialogPersistentSettings &GetInstance();

protected:

	void			SaveExtraRegistrySettings(HKEY hKey);
	void			LoadExtraRegistrySettings(HKEY hKey);

	void			SaveExtraXMLSettings(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pParentNode);
	void			LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue);

private:

	friend CColorRuleDialog;

	static const TCHAR SETTINGS_KEY[];
	static const COLORREF DEFAULT_INITIAL_COLOR;

	CColorRuleDialogPersistentSettings();

	CColorRuleDialogPersistentSettings(const CColorRuleDialogPersistentSettings &);
	CColorRuleDialogPersistentSettings & operator=(const CColorRuleDialogPersistentSettings &);

	COLORREF	m_cfInitialColor;
	COLORREF	m_cfCustomColors[16];
};

class CColorRuleDialog : public CBaseDialog
{
public:

	CColorRuleDialog(HINSTANCE hInstance,int iResource,HWND hParent,ColorRule_t *pColorRule,BOOL bEdit);
	~CColorRuleDialog();

	LRESULT CALLBACK	StaticColorProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);

protected:

	BOOL	OnInitDialog();
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnClose();

	void	SaveState();

private:

	void	OnChangeColor();

	void	OnOk();
	void	OnCancel();

	BOOL		m_bEdit;
	ColorRule_t	*m_pColorRule;

	COLORREF	m_cfCurrentColor;

	CColorRuleDialogPersistentSettings	*m_pcrdps;
};

#endif