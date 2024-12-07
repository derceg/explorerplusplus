// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class FrequentLocationsModel;

namespace FrequentLocationsRegistryStorage
{

void Load(HKEY applicationKey, FrequentLocationsModel *model);
void Save(HKEY applicationKey, const FrequentLocationsModel *model);

}
