// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TerminalOutputParser.h"

namespace
{

constexpr std::wstring_view CURRENT_DIRECTORY_SEQUENCE_PREFIX = L"\x1b]9;9;";
constexpr size_t MAXIMUM_PENDING_OUTPUT_SIZE = 32768;

size_t GetPotentialPrefixLength(std::wstring_view output)
{
	size_t maximumLength = std::min(output.size(), CURRENT_DIRECTORY_SEQUENCE_PREFIX.size() - 1);

	for (size_t length = maximumLength; length > 0; length--)
	{
		if (output.ends_with(CURRENT_DIRECTORY_SEQUENCE_PREFIX.substr(0, length)))
		{
			return length;
		}
	}

	return 0;
}

}

TerminalOutputParser::TerminalOutputParser(
	std::function<void(const std::wstring &)> directoryChangedCallback) :
	m_directoryChangedCallback(std::move(directoryChangedCallback))
{
}

std::wstring TerminalOutputParser::Process(std::wstring_view output)
{
	m_pendingOutput.append(output);
	std::wstring filteredOutput;

	while (true)
	{
		auto sequenceStart = m_pendingOutput.find(CURRENT_DIRECTORY_SEQUENCE_PREFIX);

		if (sequenceStart == std::wstring::npos)
		{
			size_t retainedLength = GetPotentialPrefixLength(m_pendingOutput);
			filteredOutput.append(m_pendingOutput, 0, m_pendingOutput.size() - retainedLength);
			m_pendingOutput.erase(0, m_pendingOutput.size() - retainedLength);
			return filteredOutput;
		}

		if (sequenceStart > 0)
		{
			filteredOutput.append(m_pendingOutput, 0, sequenceStart);
			m_pendingOutput.erase(0, sequenceStart);
		}

		size_t valueStart = CURRENT_DIRECTORY_SEQUENCE_PREFIX.size();
		size_t stringTerminator = m_pendingOutput.find(L"\x1b\\", valueStart);
		size_t bellTerminator = m_pendingOutput.find(L'\a', valueStart);
		size_t sequenceEnd = std::min(stringTerminator, bellTerminator);

		if (sequenceEnd == std::wstring::npos)
		{
			if (m_pendingOutput.size() > MAXIMUM_PENDING_OUTPUT_SIZE)
			{
				filteredOutput.append(m_pendingOutput);
				m_pendingOutput.clear();
			}

			return filteredOutput;
		}

		m_directoryChangedCallback(m_pendingOutput.substr(valueStart, sequenceEnd - valueStart));

		size_t terminatorLength = sequenceEnd == stringTerminator ? 2 : 1;
		m_pendingOutput.erase(0, sequenceEnd + terminatorLength);
	}
}
