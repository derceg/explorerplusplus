// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

// This exists only to serve as a form of type documentation. Constants declared with this type are
// raw pixel values, specified at 96DPI. This helps to avoid having to use constant names like
// MINIMUM_WIDTH_PIXELS_96DPI.
// Note that while names in the global namespace shouldn't start with _, it appears the _px portion
// of the literal operator below isn't a name and isn't reserved (see
// https://stackoverflow.com/questions/13793996/underscores-names-and-literal-operators). Defining
// the literal in a namespace would work, but would make using it in a header unwieldy. A "using
// namespace" declaration would be required, which couldn't be directly used in the header, so
// something like an initialization lambda would be needed instead.
// Which would defeat the entire purpose of something like this - a short literal that gives
// information about the type.
// Therefore, the literal is explicitly defined outside of a namespace.
constexpr int operator"" _px(unsigned long long pixels)
{
	return static_cast<int>(pixels);
}

// Specifies a point (unit for font size) value.
constexpr int operator"" _pt(unsigned long long points)
{
	return static_cast<int>(points);
}
