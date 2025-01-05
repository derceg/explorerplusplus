// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BetterEnumsWrapper.h"
#include <wil/com.h>
#include <MsXml2.h>
#include <gdiplus.h>
#include <objbase.h>
#include <list>

namespace XMLSettings
{

wil::com_ptr_nothrow<IXMLDOMDocument> CreateXmlDocument();
HRESULT FormatXmlDocument(IXMLDOMDocument *xmlDocument);
void WriteStandardSetting(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pGrandparentNode,
	const std::wstring &elementName, const std::wstring &attributeName,
	const std::wstring &attributeValue);
VARIANT VariantString(const std::wstring &str);
void AppendChildToParent(IXMLDOMNode *pChild, IXMLDOMNode *pParent);
void AddAttributeToNode(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode,
	const std::wstring &attributeName, const std::wstring &attributeValue);
void AddStringListToNode(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode,
	const std::wstring &baseKeyName, const std::list<std::wstring> &strList);
void CreateElementNode(IXMLDOMDocument *pXMLDom, IXMLDOMElement **pParentNode,
	IXMLDOMElement *pGrandparentNode, const std::wstring &elementName,
	const std::wstring &attributeName);
std::wstring EncodeBoolValue(BOOL value);
BOOL DecodeBoolValue(const std::wstring &value);
std::wstring EncodeIntValue(int value);
int DecodeIntValue(const std::wstring &value);
COLORREF ReadXMLColorData(IXMLDOMNode *pNode);
LOGFONT ReadXMLFontData(IXMLDOMNode *pNode);

bool ReadDateTime(IXMLDOMNamedNodeMap *attributeMap, const std::wstring &baseKeyName,
	FILETIME &dateTime);
void SaveDateTime(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode,
	const std::wstring &baseKeyName, const FILETIME &dateTime);
HRESULT ReadRgb(IXMLDOMNamedNodeMap *attributeMap, COLORREF &outputValue);
void SaveRgb(IXMLDOMDocument *xmlDocument, IXMLDOMElement *parentNode, COLORREF color);
HRESULT GetIntFromMap(IXMLDOMNamedNodeMap *attributeMap, const std::wstring &name,
	int &outputValue);
HRESULT GetBoolFromMap(IXMLDOMNamedNodeMap *attributeMap, const std::wstring &name,
	bool &outputValue);
HRESULT GetStringFromMap(IXMLDOMNamedNodeMap *attributeMap, const std::wstring &name,
	std::wstring &outputValue);

template <BetterEnum T>
void LoadBetterEnumValue(IXMLDOMNamedNodeMap *attributeMap, const std::wstring &valueName,
	T &output)
{
	int value;
	HRESULT hr = GetIntFromMap(attributeMap, valueName, value);

	if (FAILED(hr) || !T::_is_valid(value))
	{
		return;
	}

	output = T::_from_integral(value);
}

}
