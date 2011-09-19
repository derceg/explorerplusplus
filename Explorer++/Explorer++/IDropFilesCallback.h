#ifndef IDROPFILESCALLBACK_INCLUDED
#define IDROPFILESCALLBACK_INCLUDED

#include "Explorer++_internal.h"
#include "../Helper/DropHandler.h"

class CDropFilesCallback : public IDropFilesCallback
{
public:

	CDropFilesCallback(IExplorerplusplus *pexpp);
	~CDropFilesCallback();

	/* IUnknown methods. */
	HRESULT __stdcall	QueryInterface(REFIID iid,void **ppvObject);
	ULONG __stdcall		AddRef(void);
	ULONG __stdcall		Release(void);

private:

	/* IDropFilesCallback methods. */
	void OnDropFile(const std::list<std::wstring> &PastedFileList,POINT *ppt);

	ULONG				m_RefCount;
	IExplorerplusplus	*m_pexpp;
};

#endif