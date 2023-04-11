// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/MovableModel.h"

template <class T>
bool operator==(const MovableModel<T> &first, const MovableModel<T> &second)
{
	if (first.GetItems().size() != second.GetItems().size())
	{
		return false;
	}

	for (size_t i = 0; i < first.GetItems().size(); i++)
	{
		if (*first.GetItemAtIndex(i) != *second.GetItemAtIndex(i))
		{
			return false;
		}
	}

	return true;
}
