#ifndef BUFFER_INCLUDED
#define BUFFER_INCLUDED

__interface IBufferManager : IUnknown
{
	virtual HRESULT		Write(TCHAR *szBuffer);
	virtual HRESULT		WriteLine(TCHAR *LineBuf);
	virtual HRESULT		WriteToFile(TCHAR *FileName);
	virtual HRESULT		WriteListEntry(TCHAR *ListEntry);
	virtual HRESULT		QueryBufferSize(DWORD *BufferSize);
	virtual HRESULT		QueryBuffer(TCHAR *DestBuf,UINT DestBufSize);
};

class CBufferManager : public IBufferManager
{
public:
	/*Constructor/Deconstructor.*/
	CBufferManager();
	~CBufferManager();

	/*IUnknown methods.*/
	HRESULT		__stdcall	QueryInterface(REFIID iid,void **ppvObject);
	ULONG		__stdcall	AddRef(void);
	ULONG		__stdcall	Release(void);

	HRESULT		Write(TCHAR *szBuffer);
	HRESULT		WriteLine(TCHAR *LineBuf);
	HRESULT		WriteToFile(TCHAR *FileName);
	HRESULT		WriteListEntry(TCHAR *ListEntry);
	HRESULT		QueryBufferSize(DWORD *BufferSize);
	HRESULT		QueryBuffer(TCHAR *DestBuf,UINT DestBufSize);

private:
	int m_iRefCount;

	TCHAR *m_Buffer;
	int m_iCurrentBufSize;
	int m_NumLines;

	HRESULT		InsertNewLine();
	HRESULT		ExtendBuffer(int iExtensionSize);
};

#endif