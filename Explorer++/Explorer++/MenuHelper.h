// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

struct MenuIdRange
{
	UINT startId;
	UINT endId;
};

void UpdateMenuAcceleratorStrings(HMENU menu, HACCEL acceleratorTable);
