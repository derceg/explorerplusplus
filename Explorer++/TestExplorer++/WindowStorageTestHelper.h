// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "StorageTestHelper.h"
#include <vector>

struct WindowStorageData;

namespace WindowStorageTestHelper
{

std::vector<WindowStorageData> BuildV2ReferenceWindows(TestStorageType storageType);
WindowStorageData BuildV2FallbackReferenceWindow(TestStorageType storageType);
WindowStorageData BuildV1ReferenceWindow(TestStorageType storageType);

}
