// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Columns.h"
#include <string>

class ResourceLoader;

std::wstring GetColumnName(const ResourceLoader *resourceLoader, ColumnType columnType);
std::wstring GetColumnDescription(const ResourceLoader *resourceLoader, ColumnType columnType);
