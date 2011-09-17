#ifndef IMODELESSDIALOGNOTIFICATION_INCLUDED
#define IMODELESSDIALOGNOTIFICATION_INCLUDED

#include "../Helper/BaseDialog.h"
#include "../Helper/ReferenceCount.h"

class CModelessDialogNotification : public CReferenceCount, public IModelessDialogNotification
{
public:

	CModelessDialogNotification();
	~CModelessDialogNotification();

	ULONG AddRef();
	ULONG Release();

private:

	/* IModelessDialogNotification methods. */
	void OnModelessDialogDestroy(int iResource);

	ULONG m_RefCount;
};

#endif