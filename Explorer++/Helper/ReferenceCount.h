#ifndef REFERENCECOUNT_INCLUDED
#define REFERENCECOUNT_INCLUDED

class CReferenceCount
{
public:

	CReferenceCount();

	ULONG	AddRef();
	ULONG	Release();

private:

	LONG	m_lRefCount;
};

#endif