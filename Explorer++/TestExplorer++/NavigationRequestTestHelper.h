// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/NavigationRequest.h"
#include <gmock/gmock.h>

// Verifies that the NavigateParams stored by the specified NavigationRequest match the expected
// NavigateParams.
MATCHER_P(NavigateParamsMatch, expectedNavigateParams, "")
{
	return arg->GetNavigateParams() == expectedNavigateParams;
}
