// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

// This should be temporary, as the warning has already been fixed in fmt, but the fix isn't yet in
// a released version. See:
//
// https://github.com/fmtlib/fmt/issues/4870
// https://github.com/fmtlib/fmt/pull/4822
//
// Note that warnings are disabled in external headers (any header that uses angle brackets), but
// that doesn't work for C4702, due to this issue:
//
// https://developercommunity.visualstudio.com/t/error:-C4702-with-external:w0/1696694
#pragma warning(push)
#pragma warning(disable : 4702) // unreachable code

#include <fmt/format.h>

#pragma warning(pop)
