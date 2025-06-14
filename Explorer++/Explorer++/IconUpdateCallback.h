// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <functional>

using IconUpdateCallback = std::function<void(wil::unique_hbitmap updatedIcon)>;
