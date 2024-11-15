// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <gtest/gtest.h>
#include <wil/com.h>
#include <msxml.h>
#include <optional>
#include <string>

class XmlStorageTest : public testing::Test
{
protected:
	struct XmlDocumentData
	{
		wil::com_ptr_nothrow<IXMLDOMDocument> xmlDocument;
		wil::com_ptr_nothrow<IXMLDOMNode> rootNode;
	};

	XmlDocumentData LoadXmlDocument(const std::wstring &filePath);
	XmlDocumentData CreateXmlDocument();

private:
	void LoadXmlDocumentHelper(const std::wstring &filePath,
		XmlDocumentData &outputXmlDocumentData);
	void CreateXmlDocumentHelper(XmlDocumentData &outputXmlDocumentData);
};
