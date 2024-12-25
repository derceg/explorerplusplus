// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

// This warning has been fixed upstream (see https://github.com/tplgy/cppcodec/issues/59), but the
// fix hasn't yet been incorporated into a release, which is why the warning is ignored here.
#pragma warning(push)
#pragma warning(disable : 4702) // unreachable code
#include <cppcodec/base64_rfc4648.hpp>
#pragma warning(pop)
