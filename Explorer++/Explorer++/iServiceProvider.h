// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"

class ServiceProvider : public IServiceProvider
{
public:

	ServiceProvider(IExplorerplusplus *pexpp);

private:

	/* IUnknown methods. */
	HRESULT __stdcall	QueryInterface(REFIID iid,void **ppvObject);
	ULONG __stdcall		AddRef(void);
	ULONG __stdcall		Release(void);

	/* IServiceProvider methods. */
	HRESULT	__stdcall	QueryService(REFGUID guidService,REFIID riid,void **ppv);

	ULONG				m_RefCount;
	IExplorerplusplus	*m_pexpp;
};