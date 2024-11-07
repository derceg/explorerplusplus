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

void Load(HKEY applicationKey, ApplicationModel *model);
void Save(HKEY applicationKey, const ApplicationModel *model);

}

}
