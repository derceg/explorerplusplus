// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * XML helper functions. Intended to be used when
 * reading from/writing to an XML file.
 */

#include "stdafx.h"
#include "XMLSettings.h"
#include "Helper.h"
#include "Macros.h"
#include <wil/com.h>
#include <comdef.h>

static const TCHAR BOOL_YES[] = _T("yes");
static const TCHAR BOOL_NO[] = _T("no");

/* Helper function to create a DOM instance. */
IXMLDOMDocument *NXMLSettings::DomFromCOM()
{
	HRESULT					hr;
	IXMLDOMDocument	*pxmldoc = nullptr;

	hr = CoCreateInstance(__uuidof(DOMDocument30),nullptr,CLSCTX_INPROC_SERVER,
		__uuidof(IXMLDOMDocument),reinterpret_cast<LPVOID *>(&pxmldoc));

	if(SUCCEEDED(hr))
	{
		pxmldoc->put_async(VARIANT_FALSE);
		pxmldoc->put_validateOnParse(VARIANT_FALSE);
		pxmldoc->put_resolveExternals(VARIANT_FALSE);
		pxmldoc->put_preserveWhiteSpace(VARIANT_TRUE);
	}

	return pxmldoc;
}

void NXMLSettings::WriteStandardSetting(IXMLDOMDocument *pXMLDom,
	IXMLDOMElement *pGrandparentNode, const TCHAR *szElementName,
	const TCHAR *szAttributeName, const TCHAR *szAttributeValue)
{
	IXMLDOMElement		*pParentNode = nullptr;
	IXMLDOMAttribute	*pa = nullptr;
	IXMLDOMAttribute	*pa1 = nullptr;
	BSTR						bstr = nullptr;
	BSTR						bstr_wsntt = SysAllocString(L"\n\t\t");
	VARIANT						var;

	bstr = SysAllocString(szElementName);
	pXMLDom->createElement(bstr,&pParentNode);
	SysFreeString(bstr);
	bstr = nullptr;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pParentNode);

	/* This will form an attribute of the form:
	name="AttributeName" */
	bstr = SysAllocString(L"name");

	var = NXMLSettings::VariantString(szAttributeName);

	pXMLDom->createAttribute(bstr,&pa);
	pa->put_value(var);
	pParentNode->setAttributeNode(pa,&pa1);
	SysFreeString(bstr);
	bstr = nullptr;

	if (pa1)
	{
		pa1->Release();
		pa1 = nullptr;
	}

	pa->Release();
	pa = nullptr;
	VariantClear(&var);

	bstr = SysAllocString(szAttributeValue);
	pParentNode->put_text(bstr);
	SysFreeString(bstr);
	bstr = nullptr;

	SysFreeString(bstr_wsntt);
	bstr_wsntt = nullptr;

	NXMLSettings::AppendChildToParent(pParentNode,pGrandparentNode);

	pParentNode->Release();
	pParentNode = nullptr;
}

VARIANT NXMLSettings::VariantString(const WCHAR *str)
{
	VARIANT	var;

	VariantInit(&var);
	V_BSTR(&var) = SysAllocString(str);
	V_VT(&var) = VT_BSTR;

	return var;
}

/* Helper function to append a whitespace text node to a
specified element. */
void NXMLSettings::AddWhiteSpaceToNode(IXMLDOMDocument* pDom,
	BSTR bstrWs, IXMLDOMNode *pNode)
{
	IXMLDOMText	*pws = nullptr;
	IXMLDOMNode	*pBuf = nullptr;

	HRESULT hr = pDom->createTextNode(bstrWs,&pws);

	if(FAILED(hr))
	{
		goto clean;
	}

	hr = pNode->appendChild(pws,&pBuf);

	if(FAILED(hr))
	{
		goto clean;
	}

clean:
	SafeRelease(&pws);
	SafeRelease(&pBuf);
}

/* Helper function to append a child to a parent node. */
void NXMLSettings::AppendChildToParent(IXMLDOMNode *pChild, IXMLDOMNode *pParent)
{
	IXMLDOMNode	*pNode = nullptr;
	pParent->appendChild(pChild, &pNode);
	SafeRelease(&pNode);
}

void NXMLSettings::AddAttributeToNode(IXMLDOMDocument *pXMLDom,
IXMLDOMElement *pParentNode,const WCHAR *wszAttributeName,
const WCHAR *wszAttributeValue)
{
	IXMLDOMAttribute	*pa = nullptr;
	IXMLDOMAttribute	*pa1 = nullptr;
	BSTR						bstr = nullptr;
	VARIANT						var;

	bstr = SysAllocString(wszAttributeName);

	var = VariantString(wszAttributeValue);

	pXMLDom->createAttribute(bstr,&pa);
	pa->put_value(var);
	pParentNode->setAttributeNode(pa,&pa1);

	SysFreeString(bstr);
	bstr = nullptr;

	if(pa1)
	{
		pa1->Release();
		pa1 = nullptr;
	}

	pa->Release();
	pa = nullptr;

	VariantClear(&var);
}

void NXMLSettings::AddStringListToNode(IXMLDOMDocument *pXMLDom,
IXMLDOMElement *pParentNode,const TCHAR *szBaseKeyName,
	const std::list<std::wstring> &strList)
{
	TCHAR szNode[64];
	int i = 0;

	for(const auto &str : strList)
	{
		StringCchPrintf(szNode,SIZEOF_ARRAY(szNode),_T("%s%d"),
			szBaseKeyName,i++);
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,
			szNode,str.c_str());
	}
}

void NXMLSettings::CreateElementNode(IXMLDOMDocument *pXMLDom,
	IXMLDOMElement **pParentNode,
	IXMLDOMElement *pGrandparentNode, const WCHAR *szElementName,
	const WCHAR *szAttributeName)
{
	BSTR bstrElement = nullptr;
	BSTR bstrName = nullptr;
	VARIANT var;
	bool varInitialized = false;
	IXMLDOMAttribute *pa = nullptr;
	IXMLDOMAttribute *pa1 = nullptr;
	HRESULT hr;

	bstrElement = SysAllocString(szElementName);

	if(bstrElement == nullptr)
	{
		goto clean;
	}

	hr = pXMLDom->createElement(bstrElement, pParentNode);

	if(FAILED(hr))
	{
		goto clean;
	}

	bstrName = SysAllocString(L"name");

	if(bstrName == nullptr)
	{
		goto clean;
	}

	var = VariantString(szAttributeName);
	varInitialized = true;

	hr = pXMLDom->createAttribute(bstrName, &pa);

	if(FAILED(hr))
	{
		goto clean;
	}

	hr = pa->put_value(var);

	if(FAILED(hr))
	{
		goto clean;
	}

	hr = (*pParentNode)->setAttributeNode(pa, &pa1);

	if(FAILED(hr))
	{
		goto clean;
	}

	AppendChildToParent(*pParentNode, pGrandparentNode);

clean:
	SafeBSTRRelease(bstrElement);
	SafeBSTRRelease(bstrName);
	if(varInitialized) { VariantClear(&var); }
	SafeRelease(&pa);
	SafeRelease(&pa1);
}

const TCHAR	*NXMLSettings::EncodeBoolValue(BOOL value)
{
	if(value)
	{
		return BOOL_YES;
	}

	return BOOL_NO;
}

BOOL NXMLSettings::DecodeBoolValue(const TCHAR *value)
{
	if(lstrcmp(value, BOOL_YES) == 0)
	{
		return TRUE;
	}

	return FALSE;
}

WCHAR *NXMLSettings::EncodeIntValue(int iValue)
{
	static WCHAR wszDest[64];

	_itow_s(iValue,wszDest,
		SIZEOF_ARRAY(wszDest),10);

	return wszDest;
}

int NXMLSettings::DecodeIntValue(const WCHAR *wszValue)
{
	return _wtoi(wszValue);
}

COLORREF NXMLSettings::ReadXMLColorData(IXMLDOMNode *pNode)
{
	IXMLDOMNode			*pChildNode = nullptr;
	IXMLDOMNamedNodeMap	*am = nullptr;
	BSTR						bstrName;
	BSTR						bstrValue;
	long						lChildNodes;
	BYTE						r = 0;
	BYTE						g = 0;
	BYTE						b = 0;
	long						i = 0;

	pNode->get_attributes(&am);

	am->get_length(&lChildNodes);

	/* RGB data requires three attributes (R,G,B). */
	/*if(lChildNodes != 3)*/

	/* Attribute name should be one of: r,g,b
	Attribute value should be a value between
	0x00 and 0xFF.
	Although color values have a bound, it does
	not need to be checked for, as each color
	value is a byte, and can only hold values
	between 0x00 and 0xFF. */
	for(i = 1;i < lChildNodes;i++)
	{
		am->get_item(i,&pChildNode);

		/* Element name. */
		pChildNode->get_nodeName(&bstrName);

		/* Element value. */
		pChildNode->get_text(&bstrValue);

		if (lstrcmp(bstrName, L"r") == 0)
		{
			r = (BYTE) NXMLSettings::DecodeIntValue(bstrValue);
		}
		else if (lstrcmp(bstrName, L"g") == 0)
		{
			g = (BYTE) NXMLSettings::DecodeIntValue(bstrValue);
		}
		else if (lstrcmp(bstrName, L"b") == 0)
		{
			b = (BYTE) NXMLSettings::DecodeIntValue(bstrValue);
		}
	}

	return RGB(r,g,b);
}

Gdiplus::Color NXMLSettings::ReadXMLColorData2(IXMLDOMNode *pNode)
{
	IXMLDOMNode			*pChildNode = nullptr;
	IXMLDOMNamedNodeMap	*am = nullptr;
	Gdiplus::Color				color;
	BSTR						bstrName;
	BSTR						bstrValue;
	long						lChildNodes;
	BYTE						r = 0;
	BYTE						g = 0;
	BYTE						b = 0;
	long						i = 0;

	pNode->get_attributes(&am);

	am->get_length(&lChildNodes);

	/* RGB data requires three attributes (R,G,B). */
	/*if(lChildNodes != 3)*/

	/* Attribute name should be one of: r,g,b
	Attribute value should be a value between 0x00 and 0xFF. */
	for(i = 1;i < lChildNodes;i++)
	{
		am->get_item(i,&pChildNode);

		/* Element name. */
		pChildNode->get_nodeName(&bstrName);

		/* Element value. */
		pChildNode->get_text(&bstrValue);

		if (lstrcmp(bstrName, L"r") == 0)
		{
			r = (BYTE) NXMLSettings::DecodeIntValue(bstrValue);
		}
		else if (lstrcmp(bstrName, L"g") == 0)
		{
			g = (BYTE) NXMLSettings::DecodeIntValue(bstrValue);
		}
		else if (lstrcmp(bstrName, L"b") == 0)
		{
			b = (BYTE) NXMLSettings::DecodeIntValue(bstrValue);
		}
	}

	color = Gdiplus::Color(r,g,b);

	return color;
}

HFONT NXMLSettings::ReadXMLFontData(IXMLDOMNode *pNode)
{
	IXMLDOMNode			*pChildNode = nullptr;
	IXMLDOMNamedNodeMap	*am = nullptr;
	LOGFONT						fontInfo;
	BSTR						bstrName;
	BSTR						bstrValue;
	long						lChildNodes;
	long						i = 0;

	pNode->get_attributes(&am);

	am->get_length(&lChildNodes);

	for(i = 1;i < lChildNodes;i++)
	{
		am->get_item(i,&pChildNode);

		pChildNode->get_nodeName(&bstrName);
		pChildNode->get_text(&bstrValue);

		if (lstrcmp(bstrName, L"Height") == 0)
		{
			fontInfo.lfHeight = NXMLSettings::DecodeIntValue(bstrValue);
		}
		else if (lstrcmp(bstrName, L"Width") == 0)
		{
			fontInfo.lfWidth = NXMLSettings::DecodeIntValue(bstrValue);
		}
		else if (lstrcmp(bstrName, L"Weight") == 0)
		{
			fontInfo.lfWeight = NXMLSettings::DecodeIntValue(bstrValue);
		}
		else if (lstrcmp(bstrName, L"Italic") == 0)
		{
			fontInfo.lfItalic = (BYTE) NXMLSettings::DecodeBoolValue(bstrValue);
		}
		else if (lstrcmp(bstrName, L"Underline") == 0)
		{
			fontInfo.lfUnderline = (BYTE) NXMLSettings::DecodeBoolValue(bstrValue);
		}
		else if (lstrcmp(bstrName, L"Strikeout") == 0)
		{
			fontInfo.lfStrikeOut = (BYTE) NXMLSettings::DecodeBoolValue(bstrValue);
		}
		else if (lstrcmp(bstrName, L"Font") == 0)
		{
			StringCchCopy(fontInfo.lfFaceName, SIZEOF_ARRAY(fontInfo.lfFaceName), bstrValue);
		}
	}

	fontInfo.lfWeight			= FW_MEDIUM;
	fontInfo.lfCharSet			= DEFAULT_CHARSET;
	fontInfo.lfClipPrecision	= CLIP_DEFAULT_PRECIS;
	fontInfo.lfEscapement		= 0;
	fontInfo.lfOrientation		= 0;
	fontInfo.lfOutPrecision		= OUT_DEFAULT_PRECIS;
	fontInfo.lfPitchAndFamily	= FIXED_PITCH|FF_MODERN;
	fontInfo.lfQuality			= PROOF_QUALITY;

	return CreateFontIndirect(&fontInfo);
}

bool NXMLSettings::ReadDateTime(IXMLDOMNamedNodeMap *attributeMap, const std::wstring &baseKeyName,
	FILETIME &dateTime)
{
	std::wstring lowDateTime;
	std::wstring highDateTime;
	HRESULT hr1 = GetStringFromMap(attributeMap, baseKeyName + L"Low", lowDateTime);
	HRESULT hr2 = GetStringFromMap(attributeMap, baseKeyName + L"High", highDateTime);

	if (FAILED(hr1) || FAILED(hr2))
	{
		return false;
	}

	dateTime.dwLowDateTime = stoul(lowDateTime);
	dateTime.dwHighDateTime = stoul(highDateTime);

	return true;
}

void NXMLSettings::SaveDateTime(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const std::wstring &baseKeyName, const FILETIME &dateTime)
{
	AddAttributeToNode(xmlDocument, parentNode, (baseKeyName + L"Low").c_str(), std::to_wstring(dateTime.dwLowDateTime).c_str());
	AddAttributeToNode(xmlDocument, parentNode, (baseKeyName + L"High").c_str(), std::to_wstring(dateTime.dwHighDateTime).c_str());
}

HRESULT NXMLSettings::GetIntFromMap(IXMLDOMNamedNodeMap *attributeMap, const std::wstring &name, int &outputValue)
{
	std::wstring outputString;
	HRESULT hr = GetStringFromMap(attributeMap, name, outputString);

	if (FAILED(hr))
	{
		return hr;
	}

	outputValue = DecodeIntValue(outputString.c_str());

	return hr;
}

HRESULT NXMLSettings::GetStringFromMap(IXMLDOMNamedNodeMap *attributeMap, const std::wstring &name, std::wstring &outputValue)
{
	wil::com_ptr<IXMLDOMNode> node;
	auto nodeName = wil::make_bstr(name.c_str());
	HRESULT hr = attributeMap->getNamedItem(nodeName.get(), &node);

	if (FAILED(hr))
	{
		return hr;
	}

	BSTR value;
	hr = node->get_text(&value);

	if (FAILED(hr))
	{
		return hr;
	}

	outputValue = _bstr_t(value);

	return hr;
}

void NXMLSettings::SafeBSTRRelease(BSTR bstr)
{
	if(bstr != nullptr)
	{
		SysFreeString(bstr);
	}
}