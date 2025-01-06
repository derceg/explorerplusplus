// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <gtest/gtest.h>

#define EXPECT_CHECK_DEATH(statement) EXPECT_DEATH(statement, "Check failed")
