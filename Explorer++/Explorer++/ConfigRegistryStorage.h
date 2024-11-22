// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

struct Config;

namespace ConfigRegistryStorage
{

inline constexpr wchar_t MAIN_FONT_KEY_NAME[] = L"MainFont";

void Load(HKEY applicationKey, Config &config);

}
