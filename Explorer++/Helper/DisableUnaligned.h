// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

// __unaligned is used with some of the ITEMIDLIST helper functions (for example, ILFindLastID()
// returns PUITEMID_CHILD). However, the attribute has no relevance on x86, x64 or ARM64. Leaving it
// defined will then trigger warning C4090 ('argument': different '__unaligned' qualifiers) in
// circumstances where an __unaligned pointer is being passed to a function expecting an aligned
// pointer. Since the attribute isn't actually useful on these platforms, it's defined away. That
// then prevents the unnecessary warnings from being generated.
// Also see
// https://stackoverflow.com/questions/50975437/when-is-the-unaligned-specifier-used-with-pointers.
#if defined(_M_IX86) || defined(_M_X64) || defined(_M_ARM64)
	#define __unaligned
#endif
