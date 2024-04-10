// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class AcceleratorManager;

void UpdateMenuAcceleratorStrings(HMENU menu, const AcceleratorManager *acceleratorManager);
std::wstring BuildAcceleratorString(const ACCEL &accelerator);

std::vector<ACCEL> TableToAcceleratorItems(HACCEL acceleratorTable);
wil::unique_haccel AcceleratorItemsToTable(const std::vector<ACCEL> &accelerators);
