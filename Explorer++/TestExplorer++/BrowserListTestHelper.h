// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <vector>

class BrowserList;
class BrowserWindow;

std::vector<BrowserWindow *> GetBrowserListAsVector(const BrowserList *browserList);
