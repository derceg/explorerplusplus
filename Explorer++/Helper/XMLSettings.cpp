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
#include <boost/lexical_cast.hpp>
#include <wil/com.h>
#include <wil/resource.h>
#include <comdef.h>

namespace
{

constexpr wchar_t BOOL_YES[] = L"yes";
constexpr wchar_t BOOL_NO[] = L"no";

}

namespace XMLSettings
{

wil::com_ptr_nothrow<IXMLDOMDocument> CreateXmlDocument()
{
	wil::com_ptr_nothrow<IXMLDOMDocument> xmlDocument;
	HRESULT hr = CoCreateInstance(CLSID_DOMDocument30, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&xmlDocument));

	if (FAILED(hr))
	{
		return nullptr;
	}

	xmlDocument->put_async(VARIANT_FALSE);
	xmlDocument->put_validateOnParse(VARIANT_FALSE);
	xmlDocument->put_resolveExternals(VARIANT_FALSE);
	xmlDocument->put_preserveWhiteSpace(VARIANT_TRUE);

	return xmlDocument;
}

void WriteStandardSetting(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pGrandparentNode,
	const std::wstring &elementName, const std::wstring &attributeName,
	const std::wstring &attributeValue)
{
	wil::com_ptr_nothrow<IXMLDOMElement> pParentNode;
	auto bstr = wil::make_bstr_nothrow(elementName.c_str());
	pXMLDom->createElement(bstr.get(), &pParentNode);

	/* This will form an attribute of the form:
	name="AttributeName" */
	bstr = wil::make_bstr_nothrow(L"name");

	wil::unique_variant var(VariantString(attributeName));

	wil::com_ptr_nothrow<IXMLDOMAttribute> pa;
	pXMLDom->createAttribute(bstr.get(), &pa);
	pa->put_value(var);

	wil::com_ptr_nothrow<IXMLDOMAttribute> pa1;
	pParentNode->setAttributeNode(pa.get(), &pa1);

	bstr = wil::make_bstr_nothrow(attributeValue.c_str());
	pParentNode->put_text(bstr.get());

	AppendChildToParent(pParentNode.get(), pGrandparentNode);
}

VARIANT VariantString(const std::wstring &str)
{
	VARIANT var;

	VariantInit(&var);
	V_BSTR(&var) = SysAllocString(str.c_str());
	V_VT(&var) = VT_BSTR;

	return var;
}

/* Helper function to append a child to a parent node. */
void AppendChildToParent(IXMLDOMNode *pChild, IXMLDOMNode *pParent)
{
	wil::com_ptr_nothrow<IXMLDOMNode> pNode;
	pParent->appendChild(pChild, &pNode);
}

void AddAttributeToNode(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode,
	const std::wstring &attributeName, const std::wstring &attributeValue)
{
	wil::com_ptr_nothrow<IXMLDOMAttribute> pa;
	auto bstr = wil::make_bstr_nothrow(attributeName.c_str());
	pXMLDom->createAttribute(bstr.get(), &pa);

	wil::unique_variant var(VariantString(attributeValue));
	pa->put_value(var);

	wil::com_ptr_nothrow<IXMLDOMAttribute> pa1;
	pParentNode->setAttributeNode(pa.get(), &pa1);
}

void AddStringListToNode(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode,
	const std::wstring &baseKeyName, const std::list<std::wstring> &strList)
{
	TCHAR szNode[64];
	int i = 0;

	for (const auto &str : strList)
	{
		StringCchPrintf(szNode, SIZEOF_ARRAY(szNode), _T("%s%d"), baseKeyName.c_str(), i++);
		AddAttributeToNode(pXMLDom, pParentNode, szNode, str.c_str());
	}
}

void CreateElementNode(IXMLDOMDocument *pXMLDom, IXMLDOMElement **pParentNode,
	IXMLDOMElement *pGrandparentNode, const std::wstring &elementName,
	const std::wstring &attributeName)
{
	auto bstrElement = wil::make_bstr_nothrow(elementName.c_str());
	HRESULT hr = pXMLDom->createElement(bstrElement.get(), pParentNode);

	if (FAILED(hr))
	{
		return;
	}

	wil::unique_variant var(VariantString(attributeName));

	wil::com_ptr_nothrow<IXMLDOMAttribute> pa;
	auto bstrName = wil::make_bstr_nothrow(L"name");
	hr = pXMLDom->createAttribute(bstrName.get(), &pa);

	if (FAILED(hr))
	{
		return;
	}

	hr = pa->put_value(var);

	if (FAILED(hr))
	{
		return;
	}

	wil::com_ptr_nothrow<IXMLDOMAttribute> pa1;
	hr = (*pParentNode)->setAttributeNode(pa.get(), &pa1);

	if (FAILED(hr))
	{
		return;
	}

	AppendChildToParent(*pParentNode, pGrandparentNode);
}

std::wstring EncodeBoolValue(BOOL value)
{
	if (value)
	{
		return BOOL_YES;
	}

	return BOOL_NO;
}

BOOL DecodeBoolValue(const std::wstring &value)
{
	if (value == BOOL_YES)
	{
		return TRUE;
	}

	return FALSE;
}

std::wstring EncodeIntValue(int value)
{
	return std::to_wstring(value);
}

int DecodeIntValue(const std::wstring &value)
{
	try
	{
		return boost::lexical_cast<int>(value);
	}
	catch (const boost::bad_lexical_cast &)
	{
		return 0;
	}
}

COLORREF ReadXMLColorData(IXMLDOMNode *pNode)
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
	for (long i = 1; i < lChildNodes; i++)
	{
		wil::com_ptr_nothrow<IXMLDOMNode> pChildNode;
		am->get_item(i, &pChildNode);

		/* Element name. */
		wil::unique_bstr bstrName;
		pChildNode->get_nodeName(&bstrName);

		/* Element value. */
		wil::unique_bstr bstrValue;
		pChildNode->get_text(&bstrValue);

		if (lstrcmp(bstrName.get(), L"r") == 0)
		{
			r = (BYTE) DecodeIntValue(bstrValue.get());
		}
		else if (lstrcmp(bstrName.get(), L"g") == 0)
		{
			g = (BYTE) DecodeIntValue(bstrValue.get());
		}
		else if (lstrcmp(bstrName.get(), L"b") == 0)
		{
			b = (BYTE) DecodeIntValue(bstrValue.get());
		}
	}

	return RGB(r, g, b);
}

LOGFONT ReadXMLFontData(IXMLDOMNode *pNode)
{
	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> am;
	pNode->get_attributes(&am);

	long lChildNodes;
	am->get_length(&lChildNodes);

	LOGFONT fontInfo;

	for (long i = 1; i < lChildNodes; i++)
	{
		wil::com_ptr_nothrow<IXMLDOMNode> pChildNode;
		am->get_item(i, &pChildNode);

		wil::unique_bstr bstrName;
		pChildNode->get_nodeName(&bstrName);

		wil::unique_bstr bstrValue;
		pChildNode->get_text(&bstrValue);

		if (lstrcmp(bstrName.get(), L"Height") == 0)
		{
			fontInfo.lfHeight = DecodeIntValue(bstrValue.get());
		}
		else if (lstrcmp(bstrName.get(), L"Width") == 0)
		{
			fontInfo.lfWidth = DecodeIntValue(bstrValue.get());
		}
		else if (lstrcmp(bstrName.get(), L"Weight") == 0)
		{
			fontInfo.lfWeight = DecodeIntValue(bstrValue.get());
		}
		else if (lstrcmp(bstrName.get(), L"Italic") == 0)
		{
			fontInfo.lfItalic = (BYTE) DecodeBoolValue(bstrValue.get());
		}
		else if (lstrcmp(bstrName.get(), L"Underline") == 0)
		{
			fontInfo.lfUnderline = (BYTE) DecodeBoolValue(bstrValue.get());
		}
		else if (lstrcmp(bstrName.get(), L"Strikeout") == 0)
		{
			fontInfo.lfStrikeOut = (BYTE) DecodeBoolValue(bstrValue.get());
		}
		else if (lstrcmp(bstrName.get(), L"Font") == 0)
		{
			StringCchCopy(fontInfo.lfFaceName, std::size(fontInfo.lfFaceName), bstrValue.get());
		}
	}

	fontInfo.lfWeight = FW_MEDIUM;
	fontInfo.lfCharSet = DEFAULT_CHARSET;
	fontInfo.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	fontInfo.lfEscapement = 0;
	fontInfo.lfOrientation = 0;
	fontInfo.lfOutPrecision = OUT_DEFAULT_PRECIS;
	fontInfo.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
	fontInfo.lfQuality = PROOF_QUALITY;

	return fontInfo;
}

bool ReadDateTime(IXMLDOMNamedNodeMap *attributeMap, const std::wstring &baseKeyName,
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

void SaveDateTime(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const std::wstring &baseKeyName, const FILETIME &dateTime)
{
	AddAttributeToNode(xmlDocument, parentNode, (baseKeyName + L"Low").c_str(),
		std::to_wstring(dateTime.dwLowDateTime).c_str());
	AddAttributeToNode(xmlDocument, parentNode, (baseKeyName + L"High").c_str(),
		std::to_wstring(dateTime.dwHighDateTime).c_str());
}

HRESULT ReadRgb(IXMLDOMNamedNodeMap *attributeMap, COLORREF &outputValue)
{
	int red;
	RETURN_IF_FAILED(GetIntFromMap(attributeMap, L"r", red));

	int green;
	RETURN_IF_FAILED(GetIntFromMap(attributeMap, L"g", green));

	int blue;
	RETURN_IF_FAILED(GetIntFromMap(attributeMap, L"b", blue));

	outputValue = RGB(red, green, blue);

	return S_OK;
}

void SaveRgb(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode, COLORREF color)
{
	AddAttributeToNode(xmlDocument, parentNode, L"r", EncodeIntValue(GetRValue(color)));
	AddAttributeToNode(xmlDocument, parentNode, L"g", EncodeIntValue(GetGValue(color)));
	AddAttributeToNode(xmlDocument, parentNode, L"b", EncodeIntValue(GetBValue(color)));
}

HRESULT GetIntFromMap(IXMLDOMNamedNodeMap *attributeMap, const std::wstring &name, int &outputValue)
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

HRESULT GetBoolFromMap(IXMLDOMNamedNodeMap *attributeMap, const std::wstring &name,
	bool &outputValue)
{
	std::wstring outputString;
	HRESULT hr = GetStringFromMap(attributeMap, name, outputString);

	if (FAILED(hr))
	{
		return hr;
	}

	outputValue = DecodeBoolValue(outputString.c_str());

	return hr;
}

HRESULT GetStringFromMap(IXMLDOMNamedNodeMap *attributeMap, const std::wstring &name,
	std::wstring &outputValue)
{
	wil::com_ptr_nothrow<IXMLDOMNode> node;
	auto nodeName = wil::make_bstr_nothrow(name.c_str());
	HRESULT hr = attributeMap->getNamedItem(nodeName.get(), &node);

	if (FAILED(hr))
	{
		return hr;
	}

	if (!node)
	{
		// When the value isn't found, S_FALSE will be returned (which technically is counted as
		// success) and the node will be null. As far as this function is concerned, that's also a
		// failure.
		return E_FAIL;
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

}
