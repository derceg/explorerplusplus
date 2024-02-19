// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

struct RebarBandStorageInfo
{
	UINT id;

	// Note that the only style property used is RBBS_BREAK (to detect whether the item should
	// appear on a new line). For compatibility with past versions, however, the style as a whole is
	// both loaded and saved.
	UINT style;

	UINT length;

	// This is only used in tests.
	bool operator==(const RebarBandStorageInfo &) const = default;
};
