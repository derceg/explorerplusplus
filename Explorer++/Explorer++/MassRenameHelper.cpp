// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MassRenameHelper.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/locale.hpp>
#include <filesystem>
#include <format>
#include <regex>

namespace
{

void ProcessCounterTokens(std::wstring &pattern, int index)
{
	static const std::wregex counterRegex(L"/(0*)N");
	std::wsmatch match;

	while (std::regex_search(pattern, match, counterRegex))
	{
		auto width = match[1].length() + 1;
		pattern.replace(match.position(), match.length(), std::format(L"{:0{}}", index + 1, width));
	}
}

}

std::wstring GetMassRenameTokenText(MassRenameToken token)
{
	switch (token)
	{
	case MassRenameToken::Filename:
		return L"/F";

	case MassRenameToken::LowercaseFilename:
		return L"/L";

	case MassRenameToken::UppercaseFilename:
		return L"/U";

	case MassRenameToken::Basename:
		return L"/B";

	case MassRenameToken::Extension:
		return L"/E";

	case MassRenameToken::Counter:
		return L"/N";
	}

	LOG(FATAL) << "Invalid MassRenameToken value";
}

std::wstring ExpandMassRenamePattern(const std::wstring &pattern, const std::wstring &filename,
	int index)
{
	auto output = pattern;

	for (auto token : MassRenameToken::_values())
	{
		auto tokenText = GetMassRenameTokenText(token);

		switch (token)
		{
		case MassRenameToken::Filename:
			boost::algorithm::replace_all(output, tokenText, filename);
			break;

		case MassRenameToken::LowercaseFilename:
			boost::algorithm::replace_all(output, tokenText, boost::locale::to_lower(filename));
			break;

		case MassRenameToken::UppercaseFilename:
			boost::algorithm::replace_all(output, tokenText, boost::locale::to_upper(filename));
			break;

		case MassRenameToken::Basename:
			boost::algorithm::replace_all(output, tokenText,
				std::filesystem::path(filename).stem().wstring());
			break;

		case MassRenameToken::Extension:
			boost::algorithm::replace_all(output, tokenText,
				std::filesystem::path(filename).extension().wstring());
			break;

		case MassRenameToken::Counter:
			ProcessCounterTokens(output, index);
			break;
		}
	}

	return output;
}
