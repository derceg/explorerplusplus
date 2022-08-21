// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <filesystem>
#include <string>

std::filesystem::path GetResourcesDirectoryPath();
std::filesystem::path GetResourcePath(const std::wstring &filename);
