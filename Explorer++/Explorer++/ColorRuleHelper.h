#ifndef COLORRULEHELPER_INCLUDED
#define COLORRULEHELPER_INCLUDED

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
}

#endif