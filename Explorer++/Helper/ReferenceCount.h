#ifndef REFERENCECOUNT_INCLUDED
#define REFERENCECOUNT_INCLUDED

__interface IReferenceCount
{
	ULONG	AddRef();
	ULONG	Release();
};

class CReferenceCount : public IReferenceCount
{
public:

	CReferenceCount();
	virtual ~CReferenceCount();

	ULONG	AddRef();
	ULONG	Release();

private:

	LONG	m_RefCount;
};

#endif