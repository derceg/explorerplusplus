// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BaseDialog.h"
#include "../Helper/ReferenceCount.h"

class CModelessDialogNotification : public CReferenceCount, public IModelessDialogNotification
{
public:

	CModelessDialogNotification();

	ULONG AddRef();
	ULONG Release();

private:

	/* IModelessDialogNotification methods. */
	void OnModelessDialogDestroy(int iResource);

	ULONG m_RefCount;
};