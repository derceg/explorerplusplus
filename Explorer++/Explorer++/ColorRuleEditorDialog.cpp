// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ColorRuleEditorDialog.h"
#include "ColorRule.h"
#include "ColorRuleModel.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "../Helper/StringHelper.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclass.h"
#include "../Helper/XMLSettings.h"
#include <wil/resource.h>

const TCHAR ColorRuleEditorDialogPersistentSettings::SETTINGS_KEY[] = _T("ColorRules");

const TCHAR ColorRuleEditorDialogPersistentSettings::SETTING_CUSTOM_COLORS[] = _T("CustomColors");

ColorRuleEditorDialog::ColorRuleEditorDialog(const ResourceLoader *resourceLoader,
	HINSTANCE resourceInstance, HWND parent, ColorRuleModel *model,
	std::unique_ptr<EditDetails> editDetails) :
	BaseDialog(resourceLoader, resourceInstance, IDD_NEW_COLOR_RULE, parent,
		DialogSizingType::None),
	m_model(model),
	m_editDetails(std::move(editDetails))
{
	m_persistentSettings = &ColorRuleEditorDialogPersistentSettings::GetInstance();
}

INT_PTR ColorRuleEditorDialog::OnInitDialog()
{
	if (m_editDetails->type == EditDetails::Type::ExistingItem)
	{
		std::wstring editText = m_resourceLoader->LoadString(IDS_EDIT_COLOR_RULE);
		SetWindowText(m_hDlg, editText.c_str());
	}

	const ColorRule *targetColorRule = m_editDetails->type == EditDetails::Type::NewItem
		? m_editDetails->newColorRule.get()
		: m_editDetails->existingColorRule;

	SetDlgItemText(m_hDlg, IDC_EDIT_DESCRIPTION, targetColorRule->GetDescription().c_str());
	SetDlgItemText(m_hDlg, IDC_EDIT_FILENAME_PATTERN, targetColorRule->GetFilterPattern().c_str());

	m_currentColor = targetColorRule->GetColor();

	if (targetColorRule->GetFilterPatternCaseInsensitive())
	{
		CheckDlgButton(m_hDlg, IDC_CHECK_CASE_INSENSITIVE, BST_CHECKED);
	}

	if (WI_IsFlagSet(targetColorRule->GetFilterAttributes(), FILE_ATTRIBUTE_COMPRESSED))
	{
		CheckDlgButton(m_hDlg, IDC_CHECK_COMPRESSED, BST_CHECKED);
	}

	if (WI_IsFlagSet(targetColorRule->GetFilterAttributes(), FILE_ATTRIBUTE_ENCRYPTED))
	{
		CheckDlgButton(m_hDlg, IDC_CHECK_ENCRYPTED, BST_CHECKED);
	}

	if (WI_IsFlagSet(targetColorRule->GetFilterAttributes(), FILE_ATTRIBUTE_ARCHIVE))
	{
		CheckDlgButton(m_hDlg, IDC_CHECK_ARCHIVE, BST_CHECKED);
	}

	if (WI_IsFlagSet(targetColorRule->GetFilterAttributes(), FILE_ATTRIBUTE_HIDDEN))
	{
		CheckDlgButton(m_hDlg, IDC_CHECK_HIDDEN, BST_CHECKED);
	}

	if (WI_IsFlagSet(targetColorRule->GetFilterAttributes(), FILE_ATTRIBUTE_NOT_CONTENT_INDEXED))
	{
		CheckDlgButton(m_hDlg, IDC_CHECK_NOT_INDEXED, BST_CHECKED);
	}

	if (WI_IsFlagSet(targetColorRule->GetFilterAttributes(), FILE_ATTRIBUTE_READONLY))
	{
		CheckDlgButton(m_hDlg, IDC_CHECK_READONLY, BST_CHECKED);
	}

	if (WI_IsFlagSet(targetColorRule->GetFilterAttributes(), FILE_ATTRIBUTE_SYSTEM))
	{
		CheckDlgButton(m_hDlg, IDC_CHECK_SYSTEM, BST_CHECKED);
	}

	HWND staticColorControl = GetDlgItem(m_hDlg, IDC_STATIC_COLOR);
	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(staticColorControl,
		std::bind_front(&ColorRuleEditorDialog::StaticColorControlProc, this)));

	SendMessage(GetDlgItem(m_hDlg, IDC_EDIT_DESCRIPTION), EM_SETSEL, 0, -1);
	SetFocus(GetDlgItem(m_hDlg, IDC_EDIT_DESCRIPTION));

	m_persistentSettings->RestoreDialogPosition(m_hDlg, false);

	return 0;
}

INT_PTR ColorRuleEditorDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (HIWORD(wParam) != 0)
	{
		switch (HIWORD(wParam))
		{
		case STN_DBLCLK:
			OnChangeColor();
			break;
		}
	}
	else
	{
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON_CHANGE_COLOR:
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

void ColorRuleEditorDialog::OnOk()
{
	auto description = GetWindowString(GetDlgItem(m_hDlg, IDC_EDIT_DESCRIPTION));
	auto filenamePattern = GetWindowString(GetDlgItem(m_hDlg, IDC_EDIT_FILENAME_PATTERN));

	COLORREF color = m_currentColor;

	bool caseInsensitive = (IsDlgButtonChecked(m_hDlg, IDC_CHECK_CASE_INSENSITIVE) == BST_CHECKED);

	DWORD attributes = 0;

	if (IsDlgButtonChecked(m_hDlg, IDC_CHECK_COMPRESSED) == BST_CHECKED)
	{
		WI_SetFlag(attributes, FILE_ATTRIBUTE_COMPRESSED);
	}

	if (IsDlgButtonChecked(m_hDlg, IDC_CHECK_ENCRYPTED) == BST_CHECKED)
	{
		WI_SetFlag(attributes, FILE_ATTRIBUTE_ENCRYPTED);
	}

	if (IsDlgButtonChecked(m_hDlg, IDC_CHECK_ARCHIVE) == BST_CHECKED)
	{
		WI_SetFlag(attributes, FILE_ATTRIBUTE_ARCHIVE);
	}

	if (IsDlgButtonChecked(m_hDlg, IDC_CHECK_HIDDEN) == BST_CHECKED)
	{
		WI_SetFlag(attributes, FILE_ATTRIBUTE_HIDDEN);
	}

	if (IsDlgButtonChecked(m_hDlg, IDC_CHECK_NOT_INDEXED) == BST_CHECKED)
	{
		WI_SetFlag(attributes, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
	}

	if (IsDlgButtonChecked(m_hDlg, IDC_CHECK_READONLY) == BST_CHECKED)
	{
		WI_SetFlag(attributes, FILE_ATTRIBUTE_READONLY);
	}

	if (IsDlgButtonChecked(m_hDlg, IDC_CHECK_SYSTEM) == BST_CHECKED)
	{
		WI_SetFlag(attributes, FILE_ATTRIBUTE_SYSTEM);
	}

	ApplyEdits(description, filenamePattern, caseInsensitive, attributes, color);

	EndDialog(m_hDlg, 1);
}

void ColorRuleEditorDialog::ApplyEdits(std::wstring newDescription, std::wstring newFilterPattern,
	bool newFilterPatternCaseInsensitive, DWORD newFilterAttributes, COLORREF newColor)
{
	ColorRule *targetColorRule = m_editDetails->type == EditDetails::Type::NewItem
		? m_editDetails->newColorRule.get()
		: m_editDetails->existingColorRule;

	targetColorRule->SetDescription(newDescription);
	targetColorRule->SetFilterPattern(newFilterPattern);
	targetColorRule->SetFilterPatternCaseInsensitive(newFilterPatternCaseInsensitive);
	targetColorRule->SetFilterAttributes(newFilterAttributes);
	targetColorRule->SetColor(newColor);

	if (m_editDetails->type == EditDetails::Type::NewItem)
	{
		size_t index;
		size_t numItems = m_model->GetItems().size();

		if (m_editDetails->index.has_value() && m_editDetails->index.value() <= numItems)
		{
			index = m_editDetails->index.value();
		}
		else
		{
			index = numItems;
		}

		m_model->AddItem(std::move(m_editDetails->newColorRule), index);
	}
}

void ColorRuleEditorDialog::OnCancel()
{
	EndDialog(m_hDlg, 0);
}

INT_PTR ColorRuleEditorDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
}

void ColorRuleEditorDialog::SaveState()
{
	m_persistentSettings->SaveDialogPosition(m_hDlg);

	m_persistentSettings->m_bStateSaved = TRUE;
}

void ColorRuleEditorDialog::OnChangeColor()
{
	CHOOSECOLOR chooseColor = {};
	chooseColor.lStructSize = sizeof(chooseColor);
	chooseColor.hwndOwner = m_hDlg;
	chooseColor.rgbResult = m_currentColor;
	chooseColor.lpCustColors = m_persistentSettings->m_customColors;
	chooseColor.Flags = CC_RGBINIT;
	BOOL res = ChooseColor(&chooseColor);

	if (res)
	{
		m_currentColor = chooseColor.rgbResult;

		InvalidateRect(GetDlgItem(m_hDlg, IDC_STATIC_COLOR), nullptr, TRUE);
	}
}

LRESULT ColorRuleEditorDialog::StaticColorControlProc(HWND hwnd, UINT msg, WPARAM wParam,
	LPARAM lParam)
{
	switch (msg)
	{
	case WM_ERASEBKGND:
	{
		HDC hdc = reinterpret_cast<HDC>(wParam);

		RECT rc;
		GetClientRect(hwnd, &rc);

		wil::unique_hbrush hBrush(CreateSolidBrush(m_currentColor));
		FillRect(hdc, &rc, hBrush.get());

		return 1;
	}
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

ColorRuleEditorDialogPersistentSettings::ColorRuleEditorDialogPersistentSettings() :
	DialogSettings(SETTINGS_KEY)
{
	for (size_t i = 0; i < std::size(m_customColors); i++)
	{
		m_customColors[i] = RGB(255, 255, 255);
	}
}

ColorRuleEditorDialogPersistentSettings &ColorRuleEditorDialogPersistentSettings::GetInstance()
{
	static ColorRuleEditorDialogPersistentSettings persistentSettings;
	return persistentSettings;
}

void ColorRuleEditorDialogPersistentSettings::SaveExtraRegistrySettings(HKEY hKey)
{
	RegSetValueEx(hKey, SETTING_CUSTOM_COLORS, 0, REG_BINARY,
		reinterpret_cast<LPBYTE>(&m_customColors), sizeof(m_customColors));
}

void ColorRuleEditorDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	DWORD dwSize = sizeof(m_customColors);
	RegQueryValueEx(hKey, SETTING_CUSTOM_COLORS, nullptr, nullptr,
		reinterpret_cast<LPBYTE>(&m_customColors), &dwSize);
}

void ColorRuleEditorDialogPersistentSettings::SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom,
	IXMLDOMElement *pParentNode)
{
	TCHAR szNode[32];

	for (size_t i = 0; i < std::size(m_customColors); i++)
	{
		StringCchPrintf(szNode, std::size(szNode), _T("r%d"), i);
		XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, szNode,
			XMLSettings::EncodeIntValue(GetRValue(m_customColors[i])));
		StringCchPrintf(szNode, std::size(szNode), _T("g%d"), i);
		XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, szNode,
			XMLSettings::EncodeIntValue(GetGValue(m_customColors[i])));
		StringCchPrintf(szNode, std::size(szNode), _T("b%d"), i);
		XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, szNode,
			XMLSettings::EncodeIntValue(GetBValue(m_customColors[i])));
	}
}

void ColorRuleEditorDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue)
{
	if (CheckWildcardMatch(_T("r*"), bstrName, TRUE) || CheckWildcardMatch(_T("g*"), bstrName, TRUE)
		|| CheckWildcardMatch(_T("b*"), bstrName, TRUE))
	{
		/* At the very least, the attribute name
		should reference a color component and index. */
		if (lstrlen(bstrName) < 2)
		{
			return;
		}

		int iIndex = 0;

		/* Extract the index. */
		std::wstring strIndex = bstrName;
		std::wistringstream iss(strIndex.substr(1));
		iss >> iIndex;

		if (iIndex < 0 || iIndex > (sizeof(m_customColors) - 1))
		{
			return;
		}

		COLORREF clr = m_customColors[iIndex];
		BYTE c = static_cast<BYTE>(XMLSettings::DecodeIntValue(bstrValue));

		if (CheckWildcardMatch(_T("r*"), bstrName, TRUE))
		{
			m_customColors[iIndex] = RGB(c, GetGValue(clr), GetBValue(clr));
		}
		else if (CheckWildcardMatch(_T("g*"), bstrName, TRUE))
		{
			m_customColors[iIndex] = RGB(GetRValue(clr), c, GetBValue(clr));
		}
		else if (CheckWildcardMatch(_T("b*"), bstrName, TRUE))
		{
			m_customColors[iIndex] = RGB(GetRValue(clr), GetGValue(clr), c);
		}
	}
}
