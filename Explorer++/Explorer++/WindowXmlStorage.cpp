// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "WindowXmlStorage.h"
#include "WindowStorage.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/XMLSettings.h"
#include <wil/com.h>
#include <wil/resource.h>
#include <optional>

namespace
{

namespace V2
{

const wchar_t WINDOWS_NODE_NAME[] = L"Windows";
const wchar_t WINDOW_NODE_NAME[] = L"Window";

const wchar_t SETTING_X[] = L"X";
const wchar_t SETTING_Y[] = L"Y";
const wchar_t SETTING_WIDTH[] = L"Width";
const wchar_t SETTING_HEIGHT[] = L"Height";
const wchar_t SETTING_SHOW_STATE[] = L"ShowState";

std::optional<WindowStorageData> LoadWindow(IXMLDOMNode *windowNode)
{
	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> attributeMap;
	HRESULT hr = windowNode->get_attributes(&attributeMap);

	if (hr != S_OK)
	{
		return std::nullopt;
	}

	int x;
	hr = XMLSettings::GetIntFromMap(attributeMap.get(), SETTING_X, x);

	if (hr != S_OK)
	{
		return std::nullopt;
	}

	int y;
	hr = XMLSettings::GetIntFromMap(attributeMap.get(), SETTING_Y, y);

	if (hr != S_OK)
	{
		return std::nullopt;
	}

	int width;
	hr = XMLSettings::GetIntFromMap(attributeMap.get(), SETTING_WIDTH, width);

	if (hr != S_OK)
	{
		return std::nullopt;
	}

	int height;
	hr = XMLSettings::GetIntFromMap(attributeMap.get(), SETTING_HEIGHT, height);

	if (hr != S_OK)
	{
		return std::nullopt;
	}

	WindowShowState showState = WindowShowState::Normal;
	XMLSettings::LoadBetterEnumValue(attributeMap.get(), SETTING_SHOW_STATE, showState);

	return WindowStorageData({ x, y, x + width, y + height }, showState);
}

std::vector<WindowStorageData> Load(IXMLDOMNode *windowsNode)
{
	using namespace std::string_literals;

	auto queryString = wil::make_bstr_nothrow((L"./"s + WINDOW_NODE_NAME).c_str());
	wil::com_ptr_nothrow<IXMLDOMNodeList> childNodes;
	HRESULT hr = windowsNode->selectNodes(queryString.get(), &childNodes);

	if (hr != S_OK)
	{
		return {};
	}

	wil::com_ptr_nothrow<IXMLDOMNode> childNode;
	std::vector<WindowStorageData> windows;

	while (childNodes->nextNode(&childNode) == S_OK)
	{
		auto window = LoadWindow(childNode.get());

		if (window)
		{
			windows.push_back(*window);
		}
	}

	return windows;
}

void SaveWindow(IXMLDOMDocument *xmlDocument, IXMLDOMNode *windowsNode,
	const WindowStorageData &window)
{
	wil::com_ptr_nothrow<IXMLDOMElement> windowNode;
	auto windowNodeName = wil::make_bstr_nothrow(WINDOW_NODE_NAME);
	HRESULT hr = xmlDocument->createElement(windowNodeName.get(), &windowNode);

	if (hr != S_OK)
	{
		return;
	}

	XMLSettings::AddAttributeToNode(xmlDocument, windowNode.get(), SETTING_X,
		XMLSettings::EncodeIntValue(window.bounds.left));
	XMLSettings::AddAttributeToNode(xmlDocument, windowNode.get(), SETTING_Y,
		XMLSettings::EncodeIntValue(window.bounds.top));
	XMLSettings::AddAttributeToNode(xmlDocument, windowNode.get(), SETTING_WIDTH,
		XMLSettings::EncodeIntValue(GetRectWidth(&window.bounds)));
	XMLSettings::AddAttributeToNode(xmlDocument, windowNode.get(), SETTING_HEIGHT,
		XMLSettings::EncodeIntValue(GetRectHeight(&window.bounds)));
	XMLSettings::AddAttributeToNode(xmlDocument, windowNode.get(), SETTING_SHOW_STATE,
		XMLSettings::EncodeIntValue(window.showState));

	XMLSettings::AppendChildToParent(windowNode.get(), windowsNode);
}

void Save(IXMLDOMDocument *xmlDocument, IXMLDOMNode *windowsNode,
	const std::vector<WindowStorageData> &windows)
{
	for (const auto &window : windows)
	{
		SaveWindow(xmlDocument, windowsNode, window);
	}
}

}

namespace V1
{

const wchar_t WINDOW_POSITION_NODE_NAME[] = L"WindowPosition";
const wchar_t SETTING_LEFT[] = L"NormalPositionLeft";
const wchar_t SETTING_TOP[] = L"NormalPositionTop";
const wchar_t SETTING_RIGHT[] = L"NormalPositionRight";
const wchar_t SETTING_BOTTOM[] = L"NormalPositionBottom";
const wchar_t SETTING_SHOW_STATE[] = L"ShowCmd";

std::optional<WindowStorageData> Load(IXMLDOMNode *windowPositionNode)
{
	wil::com_ptr_nothrow<IXMLDOMNode> positionSettingNode;
	auto queryString = wil::make_bstr_nothrow(L"./Setting[@name='Position']");
	HRESULT hr = windowPositionNode->selectSingleNode(queryString.get(), &positionSettingNode);

	if (hr != S_OK)
	{
		return std::nullopt;
	}

	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> attributeMap;
	hr = positionSettingNode->get_attributes(&attributeMap);

	if (hr != S_OK)
	{
		return std::nullopt;
	}

	int left;
	hr = XMLSettings::GetIntFromMap(attributeMap.get(), SETTING_LEFT, left);

	if (hr != S_OK)
	{
		return std::nullopt;
	}

	int top;
	hr = XMLSettings::GetIntFromMap(attributeMap.get(), SETTING_TOP, top);

	if (hr != S_OK)
	{
		return std::nullopt;
	}

	int right;
	hr = XMLSettings::GetIntFromMap(attributeMap.get(), SETTING_RIGHT, right);

	if (hr != S_OK)
	{
		return std::nullopt;
	}

	int bottom;
	hr = XMLSettings::GetIntFromMap(attributeMap.get(), SETTING_BOTTOM, bottom);

	if (hr != S_OK)
	{
		return std::nullopt;
	}

	int showState;
	hr = XMLSettings::GetIntFromMap(attributeMap.get(), SETTING_SHOW_STATE, showState);

	if (hr != S_OK)
	{
		return std::nullopt;
	}

	return WindowStorageData({ left, top, right, bottom }, NativeShowStateToShowState(showState));
}

}

}

namespace WindowXmlStorage
{

std::vector<WindowStorageData> Load(IXMLDOMDocument *xmlDocument)
{
	wil::com_ptr_nothrow<IXMLDOMNode> windowsNode;
	auto queryString = wil::make_bstr_nothrow(
		(std::wstring(L"/ExplorerPlusPlus/") + V2::WINDOWS_NODE_NAME).c_str());
	HRESULT hr = xmlDocument->selectSingleNode(queryString.get(), &windowsNode);

	if (hr == S_OK)
	{
		return V2::Load(windowsNode.get());
	}

	wil::com_ptr_nothrow<IXMLDOMNode> windowPositionNode;
	queryString = wil::make_bstr_nothrow(
		(std::wstring(L"/ExplorerPlusPlus/") + V1::WINDOW_POSITION_NODE_NAME).c_str());
	hr = xmlDocument->selectSingleNode(queryString.get(), &windowPositionNode);

	if (hr == S_OK)
	{
		auto window = V1::Load(windowPositionNode.get());

		if (window)
		{
			return { *window };
		}
	}

	return {};
}

void Save(IXMLDOMDocument *xmlDocument, IXMLDOMNode *rootNode,
	const std::vector<WindowStorageData> &windows)
{
	wil::com_ptr_nothrow<IXMLDOMElement> windowsNode;
	auto windowsNodeName = wil::make_bstr_nothrow(V2::WINDOWS_NODE_NAME);
	HRESULT hr = xmlDocument->createElement(windowsNodeName.get(), &windowsNode);

	if (hr != S_OK)
	{
		return;
	}

	V2::Save(xmlDocument, windowsNode.get(), windows);

	XMLSettings::AppendChildToParent(windowsNode.get(), rootNode);
}

}
