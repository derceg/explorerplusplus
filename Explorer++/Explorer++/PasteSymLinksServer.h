// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ClipboardOperations.h"
#include "PasteSymLinksServerClientBase.h"
#include <chrono>
#include <functional>

class PasteSymLinksServer : public PasteSymLinksServerClientBase
{
public:
	ClipboardOperations::PastedItems LaunchClientAndWaitForResponse(
		std::function<bool()> clientLauncher, std::chrono::milliseconds responseTimeout);
};
