// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BetterEnumsWrapper.h"
#include <map>
#include <string>

BETTER_ENUM(MassRenameToken, int,
	// Filename + extension
	Filename,

	// Filename + extension, all in lowercase
	LowercaseFilename,

	// Filename + extension, all in uppercase
	UppercaseFilename,

	// Filename without extension
	Basename,

	// Extension only
	Extension,

	// A counter, starting from 1. 0 padding can be specified by prepending 0s to the token (e.g.
	// /00N).
	Counter
)

// Returns the string representation of a token.
std::wstring GetMassRenameTokenText(MassRenameToken token);

std::wstring ExpandMassRenamePattern(const std::wstring &pattern, const std::wstring &filename,
	int index);
