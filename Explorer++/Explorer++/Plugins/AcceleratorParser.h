// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Accelerator.h"
#include <optional>
#include <string>

namespace Plugins
{
	std::optional<Accelerator> parseAccelerator(const std::wstring &acceleratorString);
}