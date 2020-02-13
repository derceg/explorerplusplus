// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

__interface IExplorerplusplus;

class ServiceProvider : public IServiceProvider
{
public:

	ServiceProvider(IExplorerplusplus *pexpp);

private:

	/* IUnknown methods. */
	HRESULT __stdcall	QueryInterface(REFIID iid,void **ppvObject) override;
	ULONG __stdcall		AddRef(void) override;
	ULONG __stdcall		Release(void) override;

	/* IServiceProvider methods. */
	HRESULT	__stdcall	QueryService(REFGUID guidService,REFIID riid,void **ppv) override;

	ULONG				m_RefCount;
	IExplorerplusplus	*m_pexpp;
};