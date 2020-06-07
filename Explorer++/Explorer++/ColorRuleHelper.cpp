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
#include <comdef.h>
#include <vector>

namespace
{
	const COLORREF CF_COMPRESSED = RGB(0,0,255);
	const COLORREF CF_ENCRYPTED = RGB(0,128,0);

	const TCHAR REG_COLORS_KEY[] = _T("Software\\Explorer++\\ColorRules");

	void LoadColorRulesFromRegistryInternal(HKEY hKey,std::vector<NColorRuleHelper::ColorRule> &ColorRules);
	void SaveColorRulesToRegistryInternal(HKEY hKey,const NColorRuleHelper::ColorRule &ColorRule,int iCount);

	void LoadColorRulesFromXMLInternal(IXMLDOMNode *pNode,std::vector<NColorRuleHelper::ColorRule> &ColorRules);
	void SaveColorRulesToXMLInternal(IXMLDOMDocument *pXMLDom,IXMLDOMElement *pe,const NColorRuleHelper::ColorRule &colorRule);
}

std::vector<NColorRuleHelper::ColorRule> NColorRuleHelper::GetDefaultColorRules()
{
	std::vector<ColorRule> colorRules;
	ColorRule colorRule;

	colorRule.strDescription		= ResourceHelper::LoadString(GetModuleHandle(nullptr), IDS_GENERAL_COLOR_RULE_COMPRESSED);
	colorRule.caseInsensitive		= FALSE;
	colorRule.rgbColour				= CF_COMPRESSED;
	colorRule.dwFilterAttributes	= FILE_ATTRIBUTE_COMPRESSED;
	colorRules.push_back(colorRule);

	colorRule.strDescription		= ResourceHelper::LoadString(GetModuleHandle(nullptr), IDS_GENERAL_COLOR_RULE_ENCRYPTED);
	colorRule.caseInsensitive		= FALSE;
	colorRule.rgbColour				= CF_ENCRYPTED;
	colorRule.dwFilterAttributes	= FILE_ATTRIBUTE_ENCRYPTED;
	colorRules.push_back(colorRule);

	return colorRules;
}

void NColorRuleHelper::LoadColorRulesFromRegistry(std::vector<NColorRuleHelper::ColorRule> &ColorRules)
{
	HKEY hKey;
	LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER,REG_COLORS_KEY,0,KEY_READ,&hKey);

	if(lRes == ERROR_SUCCESS)
	{
		ColorRules.clear();
		LoadColorRulesFromRegistryInternal(hKey,ColorRules);
		RegCloseKey(hKey);
	}
}

namespace
{
	void LoadColorRulesFromRegistryInternal(HKEY hKey,std::vector<NColorRuleHelper::ColorRule> &ColorRules)
	{
		TCHAR szKeyName[256];
		DWORD dwIndex = 0;
		DWORD dwKeyLength = SIZEOF_ARRAY(szKeyName);

		while(RegEnumKeyEx(hKey,dwIndex++,szKeyName,&dwKeyLength, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS)
		{
			HKEY hKeyChild;
			LONG res = RegOpenKeyEx(hKey,szKeyName,0,KEY_READ,&hKeyChild);

			if(res == ERROR_SUCCESS)
			{
				NColorRuleHelper::ColorRule colorRule;

				colorRule.caseInsensitive = FALSE;

				LONG lDescriptionStatus = NRegistrySettings::ReadStringFromRegistry(hKeyChild,
					_T("Description"),colorRule.strDescription);
				LONG lFilenamePatternStatus = NRegistrySettings::ReadStringFromRegistry(hKeyChild,
					_T("FilenamePattern"),colorRule.strFilterPattern);
				NRegistrySettings::ReadDwordFromRegistry(hKeyChild,_T("CaseInsensitive"),(LPDWORD)&colorRule.caseInsensitive);
				NRegistrySettings::ReadDwordFromRegistry(hKeyChild,_T("Attributes"),&colorRule.dwFilterAttributes);

				DWORD dwType = REG_BINARY;
				DWORD dwSize = sizeof(colorRule.rgbColour);
				RegQueryValueEx(hKeyChild,_T("Color"),nullptr,&dwType,reinterpret_cast<LPBYTE>(&colorRule.rgbColour),&dwSize);

				if(lDescriptionStatus == ERROR_SUCCESS && lFilenamePatternStatus == ERROR_SUCCESS)
				{
					ColorRules.push_back(colorRule);
				}

				RegCloseKey(hKeyChild);
			}

			dwKeyLength = SIZEOF_ARRAY(szKeyName);
		}
	}
}

void NColorRuleHelper::SaveColorRulesToRegistry(const std::vector<NColorRuleHelper::ColorRule> &ColorRules)
{
	SHDeleteKey(HKEY_CURRENT_USER,REG_COLORS_KEY);

	HKEY hKey;
	LONG lRes = RegCreateKeyEx(HKEY_CURRENT_USER,REG_COLORS_KEY,
		0, nullptr,REG_OPTION_NON_VOLATILE,KEY_WRITE, nullptr,&hKey, nullptr);

	if(lRes == ERROR_SUCCESS)
	{
		int iCount = 0;

		for(const auto &colorRule : ColorRules)
		{
			SaveColorRulesToRegistryInternal(hKey,colorRule,iCount);
			iCount++;
		}

		RegCloseKey(hKey);
	}
}

namespace
{
	void SaveColorRulesToRegistryInternal(HKEY hKey,const NColorRuleHelper::ColorRule &ColorRule,int iCount)
	{
		TCHAR szKeyName[32];
		_itow_s(iCount,szKeyName,SIZEOF_ARRAY(szKeyName),10);

		HKEY hKeyChild;
		LONG res = RegCreateKeyEx(hKey,szKeyName,0, nullptr,REG_OPTION_NON_VOLATILE,KEY_WRITE, nullptr,&hKeyChild, nullptr);

		if(res == ERROR_SUCCESS)
		{
			NRegistrySettings::SaveStringToRegistry(hKeyChild,_T("Description"),ColorRule.strDescription.c_str());
			NRegistrySettings::SaveStringToRegistry(hKeyChild,_T("FilenamePattern"),ColorRule.strFilterPattern.c_str());
			NRegistrySettings::SaveDwordToRegistry(hKeyChild,_T("CaseInsensitive"),ColorRule.caseInsensitive);
			NRegistrySettings::SaveDwordToRegistry(hKeyChild,_T("Attributes"),ColorRule.dwFilterAttributes);
			RegSetValueEx(hKeyChild,_T("Color"),0,REG_BINARY,reinterpret_cast<const BYTE *>(&ColorRule.rgbColour),sizeof(ColorRule.rgbColour));

			RegCloseKey(hKeyChild);
		}
	}
}

void NColorRuleHelper::LoadColorRulesFromXML(IXMLDOMDocument *pXMLDom,
	std::vector<NColorRuleHelper::ColorRule> &ColorRules)
{
	IXMLDOMNode *pNode = nullptr;
	BSTR bstr = nullptr;
	HRESULT hr;

	if(!pXMLDom)
		goto clean;

	bstr = SysAllocString(L"//ColorRule");
	hr = pXMLDom->selectSingleNode(bstr,&pNode);

	if(hr == S_OK)
	{
		ColorRules.clear();
		LoadColorRulesFromXMLInternal(pNode,ColorRules);
	}

clean:
	if (bstr) SysFreeString(bstr);
	if (pNode) pNode->Release();
}

namespace
{
	void LoadColorRulesFromXMLInternal(IXMLDOMNode *pNode,std::vector<NColorRuleHelper::ColorRule> &ColorRules)
	{
		IXMLDOMNamedNodeMap *am = nullptr;
		IXMLDOMNode *pAttributeNode = nullptr;
		IXMLDOMNode *pNextSibling = nullptr;
		NColorRuleHelper::ColorRule colorRule;
		BOOL bDescriptionFound = FALSE;
		BOOL bFilenamePatternFound = FALSE;
		BSTR bstrName;
		BSTR bstrValue;
		BYTE r = 0;
		BYTE g = 0;
		BYTE b = 0;
		HRESULT hr;
		long nAttributeNodes;

		hr = pNode->get_attributes(&am);

		if(FAILED(hr))
			return;

		am->get_length(&nAttributeNodes);

		colorRule.caseInsensitive = FALSE;

		for(long i = 0;i < nAttributeNodes;i++)
		{
			am->get_item(i, &pAttributeNode);

			pAttributeNode->get_nodeName(&bstrName);
			pAttributeNode->get_text(&bstrValue);

			if(lstrcmpi(bstrName,L"Name") == 0)
			{
				colorRule.strDescription = _bstr_t(bstrValue);

				bDescriptionFound = TRUE;
			}
			else if(lstrcmpi(bstrName,L"FilenamePattern") == 0)
			{
				colorRule.strFilterPattern = _bstr_t(bstrValue);

				bFilenamePatternFound = TRUE;
			}
			else if(lstrcmpi(bstrName,L"CaseInsensitive") == 0)
			{
				colorRule.caseInsensitive = NXMLSettings::DecodeBoolValue(bstrValue);
			}
			else if(lstrcmpi(bstrName,L"Attributes") == 0)
			{
				colorRule.dwFilterAttributes = NXMLSettings::DecodeIntValue(bstrValue);
			}
			else if(lstrcmpi(bstrName,L"r") == 0)
			{
				r = static_cast<BYTE>(NXMLSettings::DecodeIntValue(bstrValue));
			}
			else if(lstrcmpi(bstrName,L"g") == 0)
			{
				g = static_cast<BYTE>(NXMLSettings::DecodeIntValue(bstrValue));
			}
			else if(lstrcmpi(bstrName,L"b") == 0)
			{
				b = static_cast<BYTE>(NXMLSettings::DecodeIntValue(bstrValue));
			}
		}

		if(bDescriptionFound && bFilenamePatternFound)
		{
			colorRule.rgbColour = RGB(r,g,b);

			ColorRules.push_back(colorRule);
		}

		hr = pNode->get_nextSibling(&pNextSibling);

		if(hr == S_OK)
		{
			hr = pNextSibling->get_nextSibling(&pNextSibling);

			if(hr == S_OK)
			{
				LoadColorRulesFromXMLInternal(pNextSibling,ColorRules);
			}
		}
	}
}

void NColorRuleHelper::SaveColorRulesToXML(IXMLDOMDocument *pXMLDom,
IXMLDOMElement *pRoot,const std::vector<NColorRuleHelper::ColorRule> &ColorRules)
{
	IXMLDOMElement *pe = nullptr;
	BSTR bstr_wsnt = SysAllocString(L"\n\t");
	BSTR bstr;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pRoot);

	bstr = SysAllocString(L"ColorRules");
	pXMLDom->createElement(bstr,&pe);
	SysFreeString(bstr);

	for(const auto &colorRule : ColorRules)
	{
		SaveColorRulesToXMLInternal(pXMLDom,pe,colorRule);
	}

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pe);

	NXMLSettings::AppendChildToParent(pe, pRoot);
	pe->Release();
	pe = nullptr;

	SysFreeString(bstr_wsnt);
}

namespace
{
	void SaveColorRulesToXMLInternal(IXMLDOMDocument *pXMLDom,
		IXMLDOMElement *pe,const NColorRuleHelper::ColorRule &colorRule)
	{
		IXMLDOMElement *pParentNode = nullptr;
		BSTR bstr_indent;
		WCHAR wszIndent[128];
		static int iIndent = 2;

		StringCchPrintf(wszIndent,SIZEOF_ARRAY(wszIndent),L"\n");

		for(int i = 0;i < iIndent;i++)
		{
			StringCchCat(wszIndent,SIZEOF_ARRAY(wszIndent),L"\t");
		}

		bstr_indent = SysAllocString(wszIndent);

		NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_indent,pe);

		SysFreeString(bstr_indent);

		NXMLSettings::CreateElementNode(pXMLDom,&pParentNode,pe,_T("ColorRule"),colorRule.strDescription.c_str());
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("FilenamePattern"),colorRule.strFilterPattern.c_str());
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("CaseInsensitive"),NXMLSettings::EncodeBoolValue(colorRule.caseInsensitive));
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Attributes"),NXMLSettings::EncodeIntValue(colorRule.dwFilterAttributes));
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("r"),NXMLSettings::EncodeIntValue(GetRValue(colorRule.rgbColour)));
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("g"),NXMLSettings::EncodeIntValue(GetGValue(colorRule.rgbColour)));
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("b"),NXMLSettings::EncodeIntValue(GetBValue(colorRule.rgbColour)));

		pParentNode->Release();
	}
}