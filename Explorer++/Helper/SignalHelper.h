// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

enum class SlotGroup
{
	HighestPriority = 0,
	HighPriority = 1,
	Default = 2
};

// Stops signal propagation after the first successful handler (i.e. the first handler that returns
// a result that evaluates to true).
template <typename T>
struct FirstSuccessfulRequestCombiner
{
	typedef T result_type;

	template <typename InputIterator>
	T operator()(InputIterator first, InputIterator last) const
	{
		while (first != last)
		{
			if (T fullfilled = *first)
			{
				return fullfilled;
			}

			++first;
		}

		return T();
	}
};
