// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

namespace CommandLineSplitter
{

struct Result
{
	Result(const std::vector<std::string> &arguments) : succeeded(true), arguments(arguments)
	{
	}

	Result(const std::string &errorMessage) : succeeded(false), errorMessage(errorMessage)
	{
	}

	bool succeeded;
	std::vector<std::string> arguments;
	std::string errorMessage;
};

Result Split(const std::string &commandLine);

}
