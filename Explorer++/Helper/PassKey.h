// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

// Performs the same function as PassKey in the Chromium codebase (i.e. restricts a function to an
// authorized caller).
template <typename T>
class PassKey
{
private:
	friend T;
	PassKey() = default;
};
