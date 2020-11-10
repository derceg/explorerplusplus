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
#include <wil/resource.h>
#include <comdef.h>

static const TCHAR BOOL_YES[] = _T("yes");
static const TCHAR BOOL_NO[] = _T("no");

/* Helper function to create a DOM instance. */
IXMLDOMDocument *NXMLSettings::DomFromCOM()
{
	IXMLDOMDocument	*pxmldoc = nullptr;
	HRESULT hr = CoCreateInstance(__uuidof(DOMDocument30),nullptr,CLSCTX_INPROC_SERVER,
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
	wil::com_ptr_nothrow<IXMLDOMElement> pParentNode;
	auto bstr = wil::make_bstr_nothrow(szElementName);
	pXMLDom->createElement(bstr.get(),&pParentNode);

	auto bstr_wsntt = wil::make_bstr_nothrow(L"\n\t\t");
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt.get(),pParentNode.get());

	/* This will form an attribute of the form:
	name="AttributeName" */
	bstr = wil::make_bstr_nothrow(L"name");

	wil::unique_variant var(NXMLSettings::VariantString(szAttributeName));

	wil::com_ptr_nothrow<IXMLDOMAttribute> pa;
	pXMLDom->createAttribute(bstr.get(),&pa);
	pa->put_value(var);

	wil::com_ptr_nothrow<IXMLDOMAttribute> pa1;
	pParentNode->setAttributeNode(pa.get(),&pa1);

	bstr = wil::make_bstr_nothrow(szAttributeValue);
	pParentNode->put_text(bstr.get());

	NXMLSettings::AppendChildToParent(pParentNode.get(),pGrandparentNode);
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
	wil::com_ptr_nothrow<IXMLDOMText> pws;
	HRESULT hr = pDom->createTextNode(bstrWs,&pws);

	if(FAILED(hr))
	{
		return;
	}

	wil::com_ptr_nothrow<IXMLDOMNode> pBuf;
	pNode->appendChild(pws.get(),&pBuf);
}

/* Helper function to append a child to a parent node. */
void NXMLSettings::AppendChildToParent(IXMLDOMNode *pChild, IXMLDOMNode *pParent)
{
	wil::com_ptr_nothrow<IXMLDOMNode> pNode;
	pParent->appendChild(pChild, &pNode);
}

void NXMLSettings::AddAttributeToNode(IXMLDOMDocument *pXMLDom,
IXMLDOMElement *pParentNode,const WCHAR *wszAttributeName,
const WCHAR *wszAttributeValue)
{
	wil::com_ptr_nothrow<IXMLDOMAttribute> pa;
	auto bstr = wil::make_bstr_nothrow(wszAttributeName);
	pXMLDom->createAttribute(bstr.get(),&pa);

	wil::unique_variant var(VariantString(wszAttributeValue));
	pa->put_value(var);

	wil::com_ptr_nothrow<IXMLDOMAttribute> pa1;
	pParentNode->setAttributeNode(pa.get(),&pa1);
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
	auto bstrElement = wil::make_bstr_nothrow(szElementName);
	HRESULT hr = pXMLDom->createElement(bstrElement.get(), pParentNode);

	if(FAILED(hr))
	{
		return;
	}

	wil::unique_variant var(VariantString(szAttributeName));

	wil::com_ptr_nothrow<IXMLDOMAttribute> pa;
	auto bstrName = wil::make_bstr_nothrow(L"name");
	hr = pXMLDom->createAttribute(bstrName.get(), &pa);

	if(FAILED(hr))
	{
		return;
	}

	hr = pa->put_value(var);

	if(FAILED(hr))
	{
		return;
	}

	wil::com_ptr_nothrow<IXMLDOMAttribute> pa1;
	hr = (*pParentNode)->setAttributeNode(pa.get(), &pa1);

	if(FAILED(hr))
	{
		return;
	}

	AppendChildToParent(*pParentNode, pGrandparentNode);
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
	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> am;
	pNode->get_attributes(&am);

	long lChildNodes;
	am->get_length(&lChildNodes);

	/* RGB data requires three attributes (R,G,B). */
	/*if(lChildNodes != 3)*/

	BYTE r = 0;
	BYTE g = 0;
	BYTE b = 0;

	/* Attribute name should be one of: r,g,b
	Attribute value should be a value between
	0x00 and 0xFF.
	Although color values have a bound, it does
	not need to be checked for, as each color
	value is a byte, and can only hold values
	between 0x00 and 0xFF. */
	for(long i = 1;i < lChildNodes;i++)
	{
		wil::com_ptr_nothrow<IXMLDOMNode> pChildNode;
		am->get_item(i,&pChildNode);

		/* Element name. */
		wil::unique_bstr bstrName;
		pChildNode->get_nodeName(&bstrName);

		/* Element value. */
		wil::unique_bstr bstrValue;
		pChildNode->get_text(&bstrValue);

		if (lstrcmp(bstrName.get(), L"r") == 0)
		{
			r = (BYTE) NXMLSettings::DecodeIntValue(bstrValue.get());
		}
		else if (lstrcmp(bstrName.get(), L"g") == 0)
		{
			g = (BYTE) NXMLSettings::DecodeIntValue(bstrValue.get());
		}
		else if (lstrcmp(bstrName.get(), L"b") == 0)
		{
			b = (BYTE) NXMLSettings::DecodeIntValue(bstrValue.get());
		}
	}

	return RGB(r,g,b);
}

Gdiplus::Color NXMLSettings::ReadXMLColorData2(IXMLDOMNode *pNode)
{
	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> am;
	pNode->get_attributes(&am);

	long lChildNodes;
	am->get_length(&lChildNodes);

	/* RGB data requires three attributes (R,G,B). */
	/*if(lChildNodes != 3)*/

	BYTE r = 0;
	BYTE g = 0;
	BYTE b = 0;

	/* Attribute name should be one of: r,g,b
	Attribute value should be a value between 0x00 and 0xFF. */
	for(long i = 1;i < lChildNodes;i++)
	{
		wil::com_ptr_nothrow<IXMLDOMNode> pChildNode;
		am->get_item(i,&pChildNode);

		/* Element name. */
		wil::unique_bstr bstrName;
		pChildNode->get_nodeName(&bstrName);

		/* Element value. */
		wil::unique_bstr bstrValue;
		pChildNode->get_text(&bstrValue);

		if (lstrcmp(bstrName.get(), L"r") == 0)
		{
			r = (BYTE) NXMLSettings::DecodeIntValue(bstrValue.get());
		}
		else if (lstrcmp(bstrName.get(), L"g") == 0)
		{
			g = (BYTE) NXMLSettings::DecodeIntValue(bstrValue.get());
		}
		else if (lstrcmp(bstrName.get(), L"b") == 0)
		{
			b = (BYTE) NXMLSettings::DecodeIntValue(bstrValue.get());
		}
	}

	return Gdiplus::Color(r, g, b);
}

HFONT NXMLSettings::ReadXMLFontData(IXMLDOMNode *pNode)
{
	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> am;
	pNode->get_attributes(&am);

	long lChildNodes;
	am->get_length(&lChildNodes);

	LOGFONT fontInfo;

	for(long i = 1;i < lChildNodes;i++)
	{
		wil::com_ptr_nothrow<IXMLDOMNode> pChildNode;
		am->get_item(i,&pChildNode);

		wil::unique_bstr bstrName;
		pChildNode->get_nodeName(&bstrName);

		wil::unique_bstr bstrValue;
		pChildNode->get_text(&bstrValue);

		if (lstrcmp(bstrName.get(), L"Height") == 0)
		{
			fontInfo.lfHeight = NXMLSettings::DecodeIntValue(bstrValue.get());
		}
		else if (lstrcmp(bstrName.get(), L"Width") == 0)
		{
			fontInfo.lfWidth = NXMLSettings::DecodeIntValue(bstrValue.get());
		}
		else if (lstrcmp(bstrName.get(), L"Weight") == 0)
		{
			fontInfo.lfWeight = NXMLSettings::DecodeIntValue(bstrValue.get());
		}
		else if (lstrcmp(bstrName.get(), L"Italic") == 0)
		{
			fontInfo.lfItalic = (BYTE) NXMLSettings::DecodeBoolValue(bstrValue.get());
		}
		else if (lstrcmp(bstrName.get(), L"Underline") == 0)
		{
			fontInfo.lfUnderline = (BYTE) NXMLSettings::DecodeBoolValue(bstrValue.get());
		}
		else if (lstrcmp(bstrName.get(), L"Strikeout") == 0)
		{
			fontInfo.lfStrikeOut = (BYTE) NXMLSettings::DecodeBoolValue(bstrValue.get());
		}
		else if (lstrcmp(bstrName.get(), L"Font") == 0)
		{
			StringCchCopy(fontInfo.lfFaceName, SIZEOF_ARRAY(fontInfo.lfFaceName), bstrValue.get());
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
	wil::com_ptr_nothrow<IXMLDOMNode> node;
	auto nodeName = wil::make_bstr_nothrow(name.c_str());
	HRESULT hr = attributeMap->getNamedItem(nodeName.get(), &node);

	if (FAILED(hr))
	{
		return hr;
	}

	wil::unique_bstr value;
	hr = node->get_text(&value);

	if (FAILED(hr))
	{
		return hr;
	}

	outputValue = _bstr_t(value.get());

	return hr;
}