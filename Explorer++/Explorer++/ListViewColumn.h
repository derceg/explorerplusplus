// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

struct ListViewColumnId
{
	explicit constexpr ListViewColumnId(int value) : value(value)
	{
	}

	bool operator==(const ListViewColumnId &) const = default;

	int value;
};

struct ListViewColumn
{
	ListViewColumnId id;
	UINT nameStringId;
	int width;
	bool visible;

	bool operator==(const ListViewColumn &) const = default;
};
