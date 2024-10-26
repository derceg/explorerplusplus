// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ExitCode.h"
#include <optional>

namespace CommandLine
{

struct Settings;

}

namespace StartupCommandLineProcessor
{

// Processes command line options that are relevant during startup. For example, this will enable
// logging if the appropriate option has been set.
std::optional<ExitCode> Process(const CommandLine::Settings *commandLineSettings);

}
