// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BaseDialog.h"
#include "../Helper/ReferenceCount.h"

class ModelessDialogNotification : public ReferenceCount, public IModelessDialogNotification
{
public:
	ModelessDialogNotification();

	ULONG AddRef() override;
	ULONG Release() override;

private:
	/* IModelessDialogNotification methods. */
	void OnModelessDialogDestroy(int iResource) override;

	ULONG m_RefCount;
};