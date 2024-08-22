// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <string>

class Version;

namespace VersionHelper
{

enum class Channel
{
	Stable,
	Beta,
	Dev
};

enum class Platform
{
	x86,
	x64,
	Arm64
};

const Version &GetVersion();
std::wstring GetBuildDate();
Channel GetChannel();
Platform GetPlatform();

}
