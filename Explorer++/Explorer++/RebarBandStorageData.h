// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

struct RebarBandStorageData
{
	UINT id;

	// Note that this is either 0, or RBBS_BREAK. That is, this only stores information about
	// whether the band should appear on a new line. For compatibility with past versions that
	// stored the style flags, RBBS_BREAK is stored, rather than a boolean flag indicating whether
	// the item should appear on a new line.
	UINT style;

	UINT length;

	// This is only used in tests.
	bool operator==(const RebarBandStorageData &) const = default;
};
