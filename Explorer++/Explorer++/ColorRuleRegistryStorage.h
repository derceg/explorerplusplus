// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <string>

class ColorRuleModel;

namespace ColorRuleRegistryStorage
{

void Load(HKEY applicationKey, ColorRuleModel *model);
void Save(HKEY applicationKey, const ColorRuleModel *model);

}
