// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ClipboardOperations.h"
#include "PasteSymLinksServerClientBase.h"

class PasteSymLinksClient : public PasteSymLinksServerClientBase
{
public:
	void NotifyServerOfResult(const ClipboardOperations::PastedItems &pastedItems);
};
