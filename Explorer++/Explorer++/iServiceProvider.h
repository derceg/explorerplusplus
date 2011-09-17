#ifndef ISERVICEPROVIDER_INCLUDED
#define ISERVICEPROVIDER_INCLUDED

#include "Explorer++_internal.h"

class CServiceProvider : public IServiceProvider
{
public:

	CServiceProvider(IExplorerplusplus *pexpp);
	~CServiceProvider();

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

#endif