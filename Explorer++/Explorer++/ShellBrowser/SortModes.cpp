// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SortModes.h"

SortDirection InvertSortDirection(SortDirection direction)
{
	if (direction == +SortDirection::Ascending)
	{
		return SortDirection::Descending;
	}
	else
	{
		return SortDirection::Ascending;
	}
}
