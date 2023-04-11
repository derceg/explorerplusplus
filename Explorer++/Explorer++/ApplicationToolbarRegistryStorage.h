// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <string>

namespace Applications
{

class ApplicationModel;

namespace ApplicationToolbarRegistryStorage
{

void Load(const std::wstring &mainKeyPath, ApplicationModel *model);
void Save(const std::wstring &mainKeyPath, const ApplicationModel *model);

}

}
