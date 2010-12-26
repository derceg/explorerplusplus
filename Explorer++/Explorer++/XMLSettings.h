#ifndef XMLSETTINGS_INCLUDED
#define XMLSETTINGS_INCLUDED

void	CreateElementNode(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement **pParentNode,MSXML2::IXMLDOMElement *pGrandparentNode,WCHAR *szElementName,WCHAR *szAttributeName);
void	AddAttributeToNode(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pParentNode,const WCHAR *wszAttributeName,const WCHAR *wszAttributeValue);
void	AddWhiteSpaceToNode(MSXML2::IXMLDOMDocument* pDom,BSTR bstrWs,MSXML2::IXMLDOMNode *pNode);
WCHAR	*EncodeBoolValue(BOOL bValue);
BOOL	DecodeBoolValue(WCHAR *wszValue);
WCHAR	*EncodeIntValue(int iValue);
int		DecodeIntValue(WCHAR *wszValue);
VARIANT VariantString(const WCHAR *str);

#endif