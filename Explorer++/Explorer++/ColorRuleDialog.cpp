/******************************************************************
 *
 * Project: Explorer++
 * File: ColorRuleDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the new/edit color rule dialog.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "ColorRuleDialog.h"
#include "MainResource.h"
#include "../Helper/Helper.h"
#include "../Helper/XMLSettings.h"


namespace NColorRuleDialog
{
	LRESULT CALLBACK StaticColorProcStub(HWND hwnd,UINT uMsg,
		WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);
}

const TCHAR CColorRuleDialogPersistentSettings::SETTINGS_KEY[] = _T("ColorRules");
const COLORREF CColorRuleDialogPersistentSettings::DEFAULT_INITIAL_COLOR = RGB(0,94,138);

CColorRuleDialog::CColorRuleDialog(HINSTANCE hInstance,
	int iResource,HWND hParent,ColorRule_t *pColorRule,BOOL bEdit) :
CBaseDialog(hInstance,iResource,hParent,false)
{
	m_pColorRule = pColorRule;
	m_bEdit = bEdit;

	m_pcrdps = &CColorRuleDialogPersistentSettings::GetInstance();
}

CColorRuleDialog::~CColorRuleDialog()
{

}

BOOL CColorRuleDialog::OnInitDialog()
{
	if(m_bEdit)
	{
		SetDlgItemText(m_hDlg,IDC_EDIT_DESCRIPTION,m_pColorRule->strDescription.c_str());
		SetDlgItemText(m_hDlg,IDC_EDIT_FILENAMEPATTERN,m_pColorRule->strFilterPattern.c_str());

		m_cfCurrentColor = m_pColorRule->rgbColour;

		if(m_pColorRule->dwFilterAttributes & FILE_ATTRIBUTE_COMPRESSED)
			CheckDlgButton(m_hDlg,IDC_CHECK_COMPRESSED,BST_CHECKED);

		if(m_pColorRule->dwFilterAttributes & FILE_ATTRIBUTE_ENCRYPTED)
			CheckDlgButton(m_hDlg,IDC_CHECK_ENCRYPTED,BST_CHECKED);

		if(m_pColorRule->dwFilterAttributes & FILE_ATTRIBUTE_ARCHIVE)
			CheckDlgButton(m_hDlg,IDC_CHECK_ARCHIVE,BST_CHECKED);

		if(m_pColorRule->dwFilterAttributes & FILE_ATTRIBUTE_HIDDEN)
			CheckDlgButton(m_hDlg,IDC_CHECK_HIDDEN,BST_CHECKED);

		if(m_pColorRule->dwFilterAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)
			CheckDlgButton(m_hDlg,IDC_CHECK_INDEXED,BST_CHECKED);

		if(m_pColorRule->dwFilterAttributes & FILE_ATTRIBUTE_READONLY)
			CheckDlgButton(m_hDlg,IDC_CHECK_READONLY,BST_CHECKED);

		if(m_pColorRule->dwFilterAttributes & FILE_ATTRIBUTE_SYSTEM)
			CheckDlgButton(m_hDlg,IDC_CHECK_SYSTEM,BST_CHECKED);

		TCHAR szTemp[64];
		LoadString(GetInstance(),IDS_EDITCOLORRULE,
			szTemp,SIZEOF_ARRAY(szTemp));
		SetWindowText(m_hDlg,szTemp);
	}
	else
	{
		m_cfCurrentColor = m_pcrdps->m_cfInitialColor;
	}

	HWND hStaticColor = GetDlgItem(m_hDlg,IDC_STATIC_COLOR);
	SetWindowSubclass(hStaticColor,NColorRuleDialog::StaticColorProcStub,0,reinterpret_cast<DWORD_PTR>(this));

	SendMessage(GetDlgItem(m_hDlg,IDC_EDIT_DESCRIPTION),EM_SETSEL,0,-1);
	SetFocus(GetDlgItem(m_hDlg,IDC_EDIT_DESCRIPTION));

	m_pcrdps->RestoreDialogPosition(m_hDlg,false);

	return 0;
}

BOOL CColorRuleDialog::OnCommand(WPARAM wParam,LPARAM lParam)
{
	if(HIWORD(wParam) != 0)
	{
		switch(HIWORD(wParam))
		{
		case STN_DBLCLK:
			OnChangeColor();
			break;
		}
	}
	else
	{
		switch(LOWORD(wParam))
		{
		case IDC_BUTTON_CHANGECOLOR:
			OnChangeColor();
			break;

		case IDOK:
			OnOk();
			break;

		case IDCANCEL:
			OnCancel();
			break;
		}
	}

	return 0;
}

void CColorRuleDialog::OnOk()
{
	GetWindowString(GetDlgItem(m_hDlg,IDC_EDIT_DESCRIPTION),m_pColorRule->strDescription);
	GetWindowString(GetDlgItem(m_hDlg,IDC_EDIT_FILENAMEPATTERN),m_pColorRule->strFilterPattern);

	m_pColorRule->rgbColour = m_cfCurrentColor;

	m_pColorRule->dwFilterAttributes = 0;

	if(IsDlgButtonChecked(m_hDlg,IDC_CHECK_COMPRESSED) == BST_CHECKED)
		m_pColorRule->dwFilterAttributes |= FILE_ATTRIBUTE_COMPRESSED;

	if(IsDlgButtonChecked(m_hDlg,IDC_CHECK_ENCRYPTED) == BST_CHECKED)
		m_pColorRule->dwFilterAttributes |= FILE_ATTRIBUTE_ENCRYPTED;

	if(IsDlgButtonChecked(m_hDlg,IDC_CHECK_ARCHIVE) == BST_CHECKED)
		m_pColorRule->dwFilterAttributes |= FILE_ATTRIBUTE_ARCHIVE;

	if(IsDlgButtonChecked(m_hDlg,IDC_CHECK_HIDDEN) == BST_CHECKED)
		m_pColorRule->dwFilterAttributes |= FILE_ATTRIBUTE_HIDDEN;

	if(IsDlgButtonChecked(m_hDlg,IDC_CHECK_READONLY) == BST_CHECKED)
		m_pColorRule->dwFilterAttributes |= FILE_ATTRIBUTE_READONLY;

	if(IsDlgButtonChecked(m_hDlg,IDC_CHECK_SYSTEM) == BST_CHECKED)
		m_pColorRule->dwFilterAttributes |= FILE_ATTRIBUTE_SYSTEM;

	EndDialog(m_hDlg,1);
}

void CColorRuleDialog::OnCancel()
{
	EndDialog(m_hDlg,0);
}

BOOL CColorRuleDialog::OnClose()
{
	EndDialog(m_hDlg,0);
	return 0;
}

void CColorRuleDialog::SaveState()
{
	m_pcrdps->SaveDialogPosition(m_hDlg);

	m_pcrdps->m_bStateSaved = TRUE;
}

void CColorRuleDialog::OnChangeColor()
{
	CHOOSECOLOR cc;
	cc.lStructSize	= sizeof(cc);
	cc.hwndOwner	= m_hDlg;
	cc.rgbResult	= m_cfCurrentColor;
	cc.lpCustColors	= m_pcrdps->m_cfCustomColors;
	cc.Flags		= CC_RGBINIT;
	BOOL bRet = ChooseColor(&cc);

	if(bRet)
	{
		m_cfCurrentColor = cc.rgbResult;

		/* If this is a new item been created, store the color
		regardless of whether the item is actually created or
		not. */
		if(!m_bEdit)
			m_pcrdps->m_cfInitialColor = cc.rgbResult;

		InvalidateRect(GetDlgItem(m_hDlg,IDC_STATIC_COLOR),NULL,TRUE);
	}
}

LRESULT CALLBACK NColorRuleDialog::StaticColorProcStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	CColorRuleDialog *pcrd = reinterpret_cast<CColorRuleDialog *>(dwRefData);

	return pcrd->StaticColorProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK CColorRuleDialog::StaticColorProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
	case WM_ERASEBKGND:
		{
			HDC hdc = reinterpret_cast<HDC>(wParam);

			RECT rc;
			GetClientRect(hwnd,&rc);

			HBRUSH hBrush = CreateSolidBrush(m_cfCurrentColor);
			FillRect(hdc,&rc,hBrush);
			DeleteObject(hBrush);

			return 1;
		}
		break;
	}

	return DefSubclassProc(hwnd,Msg,wParam,lParam);
}

CColorRuleDialogPersistentSettings::CColorRuleDialogPersistentSettings() :
CDialogSettings(SETTINGS_KEY)
{
	m_cfInitialColor = DEFAULT_INITIAL_COLOR;

	for(int i = 0;i < SIZEOF_ARRAY(m_cfCustomColors);i++)
	{
		m_cfCustomColors[i] = RGB(255,255,255);
	}
}

CColorRuleDialogPersistentSettings::~CColorRuleDialogPersistentSettings()
{
	
}

CColorRuleDialogPersistentSettings& CColorRuleDialogPersistentSettings::GetInstance()
{
	static CColorRuleDialogPersistentSettings sfadps;
	return sfadps;
}

void CColorRuleDialogPersistentSettings::SaveExtraRegistrySettings(HKEY hKey)
{
	RegSetValueEx(hKey,_T("InitialColor"),0,REG_BINARY,
		reinterpret_cast<LPBYTE>(&m_cfInitialColor),
		sizeof(m_cfInitialColor));

	RegSetValueEx(hKey,_T("CustomColors"),0,REG_BINARY,
		reinterpret_cast<LPBYTE>(&m_cfCustomColors),
		sizeof(m_cfCustomColors));
}

void CColorRuleDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	DWORD dwSize = sizeof(m_cfInitialColor);
	RegQueryValueEx(hKey,_T("InitialColor"),NULL,NULL,
		reinterpret_cast<LPBYTE>(&m_cfInitialColor),&dwSize);

	dwSize = sizeof(m_cfCustomColors);
	RegQueryValueEx(hKey,_T("CustomColors"),NULL,NULL,
		reinterpret_cast<LPBYTE>(&m_cfCustomColors),&dwSize);
}

void CColorRuleDialogPersistentSettings::SaveExtraXMLSettings(
	MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pParentNode)
{
	TCHAR szNode[32];

	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("InitialColor_r"),NXMLSettings::EncodeIntValue(GetRValue(m_cfInitialColor)));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("InitialColor_g"),NXMLSettings::EncodeIntValue(GetGValue(m_cfInitialColor)));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("InitialColor_b"),NXMLSettings::EncodeIntValue(GetBValue(m_cfInitialColor)));

	for(int i = 0;i < SIZEOF_ARRAY(m_cfCustomColors);i++)
	{
		StringCchPrintf(szNode,SIZEOF_ARRAY(szNode),_T("r%d"),i);
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,szNode,NXMLSettings::EncodeIntValue(GetRValue(m_cfCustomColors[i])));
		StringCchPrintf(szNode,SIZEOF_ARRAY(szNode),_T("g%d"),i);
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,szNode,NXMLSettings::EncodeIntValue(GetGValue(m_cfCustomColors[i])));
		StringCchPrintf(szNode,SIZEOF_ARRAY(szNode),_T("b%d"),i);
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,szNode,NXMLSettings::EncodeIntValue(GetBValue(m_cfCustomColors[i])));
	}
}

void CColorRuleDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName,BSTR bstrValue)
{
	if(CheckWildcardMatch(_T("r*"),bstrName,TRUE) ||
		CheckWildcardMatch(_T("g*"),bstrName,TRUE) ||
		CheckWildcardMatch(_T("b*"),bstrName,TRUE))
	{
		/* At the very least, the attribute name
		should reference a color component and index. */
		if(lstrlen(bstrName) < 2)
		{
			return ;
		}

		int iIndex = 0;

		/* Extract the index. */
		std::wstring strIndex = bstrName;
		std::wistringstream iss(strIndex.substr(1));
		iss >> iIndex;

		if(iIndex < 0 || iIndex > (sizeof(m_cfCustomColors) - 1))
		{
			return;
		}

		COLORREF clr = m_cfCustomColors[iIndex];
		BYTE c = static_cast<BYTE>(NXMLSettings::DecodeIntValue(bstrValue));

		if(CheckWildcardMatch(_T("r*"),bstrName,TRUE))
			m_cfCustomColors[iIndex] = RGB(c,GetGValue(clr),GetBValue(clr));
		else if(CheckWildcardMatch(_T("g*"),bstrName,TRUE))
			m_cfCustomColors[iIndex] = RGB(GetRValue(clr),c,GetBValue(clr));
		else if(CheckWildcardMatch(_T("b*"),bstrName,TRUE))
			m_cfCustomColors[iIndex] = RGB(GetRValue(clr),GetGValue(clr),c);
	}
	else
	{
		BYTE c = static_cast<BYTE>(NXMLSettings::DecodeIntValue(bstrValue));

		if(lstrcmpi(_T("InitialColor_r"),bstrName) == 0)
			m_cfInitialColor = RGB(c,GetGValue(m_cfInitialColor),GetBValue(m_cfInitialColor));
		else if(lstrcmpi(_T("InitialColor_g"),bstrName) == 0)
			m_cfInitialColor = RGB(c,GetGValue(m_cfInitialColor),GetBValue(m_cfInitialColor));
		else if(lstrcmpi(_T("InitialColor_b"),bstrName) == 0)
			m_cfInitialColor = RGB(c,GetGValue(m_cfInitialColor),GetBValue(m_cfInitialColor));
	}
}