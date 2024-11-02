// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <concurrencpp/concurrencpp.h>
#include <vector>

// This is useful in tests, as Google Test only currently supports matching STL-style containers.
// Although concurrencpp::generator has begin() and end() iterators, it doesn't have the same
// interface as an STL-style container. Using this, the output from a generator can be matched
// within a test.
// If Google Test adds support for broader container-like matching, this function can be removed.
template <typename T>
std::vector<T> GeneratorToVector(concurrencpp::generator<T> generator)
{
	std::vector<T> items;

	for (const auto &item : generator)
	{
		items.push_back(item);
	}

	return items;
}
