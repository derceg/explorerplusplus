#ifndef COLORRULEHELPER_INCLUDED
#define COLORRULEHELPER_INCLUDED

#import <msxml3.dll> raw_interfaces_only

namespace NColorRuleHelper
{
	struct ColorRule_t
	{
		std::wstring	strDescription;

		/* Filename and attribute filtering. */
		std::wstring	strFilterPattern;
		DWORD			dwFilterAttributes;

		COLORREF		rgbColour;
	};

	std::vector<ColorRule_t> GetDefaultColorRules(HMODULE hLanguageModule);

	void	LoadColorRulesFromRegistry(std::vector<ColorRule_t> &ColorRules);
	void	SaveColorRulesToRegistry(const std::vector<ColorRule_t> &ColorRules);

	void	LoadColorRulesFromXML(MSXML2::IXMLDOMDocument *pXMLDom,std::vector<ColorRule_t> &ColorRules);
	void	SaveColorRulesToXML(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pRoot,const std::vector<ColorRule_t> &ColorRules);
}

#endif