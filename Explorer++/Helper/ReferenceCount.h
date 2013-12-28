#pragma once

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