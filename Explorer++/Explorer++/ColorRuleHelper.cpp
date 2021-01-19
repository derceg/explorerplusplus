// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ColorRuleHelper.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/XMLSettings.h"
#include <wil/com.h>
#include <comdef.h>
#include <vector>

namespace
{
	const COLORREF CF_COMPRESSED = RGB(0, 0, 255);
	const COLORREF CF_ENCRYPTED = RGB(0, 128, 0);

	const TCHAR REG_COLORS_KEY[] = _T("Software\\Explorer++\\ColorRules");

	void LoadColorRulesFromRegistryInternal(
		HKEY hKey, std::vector<NColorRuleHelper::ColorRule> &ColorRules);
	void SaveColorRulesToRegistryInternal(
		HKEY hKey, const NColorRuleHelper::ColorRule &ColorRule, int iCount);

	void LoadColorRulesFromXMLInternal(
		IXMLDOMNode *pNode, std::vector<NColorRuleHelper::ColorRule> &ColorRules);
	void SaveColorRulesToXMLInternal(
		IXMLDOMDocument *pXMLDom, IXMLDOMElement *pe, const NColorRuleHelper::ColorRule &colorRule);
}

std::vector<NColorRuleHelper::ColorRule> NColorRuleHelper::GetDefaultColorRules()
{
	std::vector<ColorRule> colorRules;
	ColorRule colorRule;

	colorRule.strDescription =
		ResourceHelper::LoadString(GetModuleHandle(nullptr), IDS_GENERAL_COLOR_RULE_COMPRESSED);
	colorRule.caseInsensitive = FALSE;
	colorRule.rgbColour = CF_COMPRESSED;
	colorRule.dwFilterAttributes = FILE_ATTRIBUTE_COMPRESSED;
	colorRules.push_back(colorRule);

	colorRule.strDescription =
		ResourceHelper::LoadString(GetModuleHandle(nullptr), IDS_GENERAL_COLOR_RULE_ENCRYPTED);
	colorRule.caseInsensitive = FALSE;
	colorRule.rgbColour = CF_ENCRYPTED;
	colorRule.dwFilterAttributes = FILE_ATTRIBUTE_ENCRYPTED;
	colorRules.push_back(colorRule);

	return colorRules;
}

void NColorRuleHelper::LoadColorRulesFromRegistry(
	std::vector<NColorRuleHelper::ColorRule> &ColorRules)
{
	HKEY hKey;
	LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER, REG_COLORS_KEY, 0, KEY_READ, &hKey);

	if (lRes == ERROR_SUCCESS)
	{
		ColorRules.clear();
		LoadColorRulesFromRegistryInternal(hKey, ColorRules);
		RegCloseKey(hKey);
	}
}

namespace
{
	void LoadColorRulesFromRegistryInternal(
		HKEY hKey, std::vector<NColorRuleHelper::ColorRule> &ColorRules)
	{
		TCHAR szKeyName[256];
		DWORD dwIndex = 0;
		DWORD dwKeyLength = SIZEOF_ARRAY(szKeyName);

		while (RegEnumKeyEx(
				   hKey, dwIndex++, szKeyName, &dwKeyLength, nullptr, nullptr, nullptr, nullptr)
			== ERROR_SUCCESS)
		{
			HKEY hKeyChild;
			LONG res = RegOpenKeyEx(hKey, szKeyName, 0, KEY_READ, &hKeyChild);

			if (res == ERROR_SUCCESS)
			{
				NColorRuleHelper::ColorRule colorRule;

				colorRule.caseInsensitive = FALSE;

				LONG lDescriptionStatus = RegistrySettings::ReadString(
					hKeyChild, _T("Description"), colorRule.strDescription);
				LONG lFilenamePatternStatus = RegistrySettings::ReadString(
					hKeyChild, _T("FilenamePattern"), colorRule.strFilterPattern);
				RegistrySettings::ReadDword(
					hKeyChild, _T("CaseInsensitive"), (LPDWORD) &colorRule.caseInsensitive);
				RegistrySettings::ReadDword(
					hKeyChild, _T("Attributes"), &colorRule.dwFilterAttributes);

				DWORD dwType = REG_BINARY;
				DWORD dwSize = sizeof(colorRule.rgbColour);
				RegQueryValueEx(hKeyChild, _T("Color"), nullptr, &dwType,
					reinterpret_cast<LPBYTE>(&colorRule.rgbColour), &dwSize);

				if (lDescriptionStatus == ERROR_SUCCESS && lFilenamePatternStatus == ERROR_SUCCESS)
				{
					ColorRules.push_back(colorRule);
				}

				RegCloseKey(hKeyChild);
			}

			dwKeyLength = SIZEOF_ARRAY(szKeyName);
		}
	}
}

void NColorRuleHelper::SaveColorRulesToRegistry(
	const std::vector<NColorRuleHelper::ColorRule> &ColorRules)
{
	SHDeleteKey(HKEY_CURRENT_USER, REG_COLORS_KEY);

	HKEY hKey;
	LONG lRes = RegCreateKeyEx(HKEY_CURRENT_USER, REG_COLORS_KEY, 0, nullptr,
		REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);

	if (lRes == ERROR_SUCCESS)
	{
		int iCount = 0;

		for (const auto &colorRule : ColorRules)
		{
			SaveColorRulesToRegistryInternal(hKey, colorRule, iCount);
			iCount++;
		}

		RegCloseKey(hKey);
	}
}

namespace
{
	void SaveColorRulesToRegistryInternal(
		HKEY hKey, const NColorRuleHelper::ColorRule &ColorRule, int iCount)
	{
		TCHAR szKeyName[32];
		_itow_s(iCount, szKeyName, SIZEOF_ARRAY(szKeyName), 10);

		HKEY hKeyChild;
		LONG res = RegCreateKeyEx(hKey, szKeyName, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE,
			nullptr, &hKeyChild, nullptr);

		if (res == ERROR_SUCCESS)
		{
			RegistrySettings::SaveString(
				hKeyChild, _T("Description"), ColorRule.strDescription.c_str());
			RegistrySettings::SaveString(
				hKeyChild, _T("FilenamePattern"), ColorRule.strFilterPattern.c_str());
			RegistrySettings::SaveDword(
				hKeyChild, _T("CaseInsensitive"), ColorRule.caseInsensitive);
			RegistrySettings::SaveDword(hKeyChild, _T("Attributes"), ColorRule.dwFilterAttributes);
			RegSetValueEx(hKeyChild, _T("Color"), 0, REG_BINARY,
				reinterpret_cast<const BYTE *>(&ColorRule.rgbColour), sizeof(ColorRule.rgbColour));

			RegCloseKey(hKeyChild);
		}
	}
}

void NColorRuleHelper::LoadColorRulesFromXML(
	IXMLDOMDocument *pXMLDom, std::vector<NColorRuleHelper::ColorRule> &ColorRules)
{
	if (!pXMLDom)
	{
		return;
	}

	wil::com_ptr_nothrow<IXMLDOMNode> pNode;
	auto bstr = wil::make_bstr_nothrow(L"//ColorRule");
	HRESULT hr = pXMLDom->selectSingleNode(bstr.get(), &pNode);

	if (hr == S_OK)
	{
		ColorRules.clear();
		LoadColorRulesFromXMLInternal(pNode.get(), ColorRules);
	}
}

namespace
{
	void LoadColorRulesFromXMLInternal(
		IXMLDOMNode *pNode, std::vector<NColorRuleHelper::ColorRule> &ColorRules)
	{
		wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> am;
		HRESULT hr = pNode->get_attributes(&am);

		if (FAILED(hr))
		{
			return;
		}

		NColorRuleHelper::ColorRule colorRule;
		colorRule.caseInsensitive = FALSE;

		BOOL bDescriptionFound = FALSE;
		BOOL bFilenamePatternFound = FALSE;
		BYTE r = 0;
		BYTE g = 0;
		BYTE b = 0;

		long nAttributeNodes;
		am->get_length(&nAttributeNodes);

		for (long i = 0; i < nAttributeNodes; i++)
		{
			wil::com_ptr_nothrow<IXMLDOMNode> pAttributeNode;
			am->get_item(i, &pAttributeNode);

			wil::unique_bstr bstrName;
			pAttributeNode->get_nodeName(&bstrName);

			wil::unique_bstr bstrValue;
			pAttributeNode->get_text(&bstrValue);

			if (lstrcmpi(bstrName.get(), L"Name") == 0)
			{
				colorRule.strDescription = _bstr_t(bstrValue.get());

				bDescriptionFound = TRUE;
			}
			else if (lstrcmpi(bstrName.get(), L"FilenamePattern") == 0)
			{
				colorRule.strFilterPattern = _bstr_t(bstrValue.get());

				bFilenamePatternFound = TRUE;
			}
			else if (lstrcmpi(bstrName.get(), L"CaseInsensitive") == 0)
			{
				colorRule.caseInsensitive = NXMLSettings::DecodeBoolValue(bstrValue.get());
			}
			else if (lstrcmpi(bstrName.get(), L"Attributes") == 0)
			{
				colorRule.dwFilterAttributes = NXMLSettings::DecodeIntValue(bstrValue.get());
			}
			else if (lstrcmpi(bstrName.get(), L"r") == 0)
			{
				r = static_cast<BYTE>(NXMLSettings::DecodeIntValue(bstrValue.get()));
			}
			else if (lstrcmpi(bstrName.get(), L"g") == 0)
			{
				g = static_cast<BYTE>(NXMLSettings::DecodeIntValue(bstrValue.get()));
			}
			else if (lstrcmpi(bstrName.get(), L"b") == 0)
			{
				b = static_cast<BYTE>(NXMLSettings::DecodeIntValue(bstrValue.get()));
			}
		}

		if (bDescriptionFound && bFilenamePatternFound)
		{
			colorRule.rgbColour = RGB(r, g, b);

			ColorRules.push_back(colorRule);
		}

		wil::com_ptr_nothrow<IXMLDOMNode> pNextSibling;
		hr = pNode->get_nextSibling(&pNextSibling);

		if (hr == S_OK)
		{
			wil::com_ptr_nothrow<IXMLDOMNode> secondSibling;
			hr = pNextSibling->get_nextSibling(&secondSibling);

			if (hr == S_OK)
			{
				LoadColorRulesFromXMLInternal(secondSibling.get(), ColorRules);
			}
		}
	}
}

void NColorRuleHelper::SaveColorRulesToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot,
	const std::vector<NColorRuleHelper::ColorRule> &ColorRules)
{
	auto bstr_wsnt = wil::make_bstr_nothrow(L"\n\t");
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsnt.get(), pRoot);

	wil::com_ptr_nothrow<IXMLDOMElement> pe;
	auto bstr = wil::make_bstr_nothrow(L"ColorRules");
	pXMLDom->createElement(bstr.get(), &pe);

	for (const auto &colorRule : ColorRules)
	{
		SaveColorRulesToXMLInternal(pXMLDom, pe.get(), colorRule);
	}

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsnt.get(), pe.get());
	NXMLSettings::AppendChildToParent(pe.get(), pRoot);
}

namespace
{
	void SaveColorRulesToXMLInternal(
		IXMLDOMDocument *pXMLDom, IXMLDOMElement *pe, const NColorRuleHelper::ColorRule &colorRule)
	{
		auto bstr_indent = wil::make_bstr_nothrow(L"\n\t\t");
		NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_indent.get(), pe);

		wil::com_ptr_nothrow<IXMLDOMElement> pParentNode;
		NXMLSettings::CreateElementNode(
			pXMLDom, &pParentNode, pe, _T("ColorRule"), colorRule.strDescription.c_str());
		NXMLSettings::AddAttributeToNode(
			pXMLDom, pParentNode.get(), _T("FilenamePattern"), colorRule.strFilterPattern.c_str());
		NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("CaseInsensitive"),
			NXMLSettings::EncodeBoolValue(colorRule.caseInsensitive));
		NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Attributes"),
			NXMLSettings::EncodeIntValue(colorRule.dwFilterAttributes));
		NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("r"),
			NXMLSettings::EncodeIntValue(GetRValue(colorRule.rgbColour)));
		NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("g"),
			NXMLSettings::EncodeIntValue(GetGValue(colorRule.rgbColour)));
		NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("b"),
			NXMLSettings::EncodeIntValue(GetBValue(colorRule.rgbColour)));
	}
}