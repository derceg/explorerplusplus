// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <vector>

struct WindowStorageData;

bool operator==(const WindowStorageData &first, const WindowStorageData &second);

namespace WindowStorageTestHelper
{

std::vector<WindowStorageData> BuildV2ReferenceWindows();
WindowStorageData BuildV1ReferenceWindow();

}
