// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "AcceleratorTestHelper.h"

bool operator==(const ACCEL &first, const ACCEL &second)
{
	return first.cmd == second.cmd && first.fVirt == second.fVirt && first.key == second.key;
}
