/******************************************************************
 *
 * Project: Helper
 * File: Buffer.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides a basic buffering system.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Buffer.h"

CBufferManager::CBufferManager()
{
	/* Initialize the reference count. This needs to be set to 1 (not 0),
	since this is the first instance of the object created. */
	m_iRefCount			= 1;

	/* This is the main buffer used to hold text. Space for it is allocated as
	neccessary. */
	m_Buffer			= NULL;

	/* Holds the current size of the buffer, including the terminating NULL
	byte (or bytes). */
	m_iCurrentBufSize	= 0;

	m_NumLines			= 0;
}

CBufferManager::~CBufferManager()
{
	free(m_Buffer);
}

/* IUnknown interface members. */
HRESULT __stdcall CBufferManager::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = NULL;

	if(iid == IID_IDataObject || iid == IID_IUnknown)
	{
		*ppvObject=this;
	}

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall CBufferManager::AddRef(void)
{
	return ++m_iRefCount;
}

ULONG __stdcall CBufferManager::Release(void)
{
	m_iRefCount--;

	if(m_iRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_iRefCount;
}

/* New lines in windows are designated by the 0X0D 0X0A sequence.
This function writes this newline tag at the end of the current
line, then writes the line buffer to the end of the buffer. */
HRESULT CBufferManager::WriteLine(TCHAR *LineBuf)
{
	TCHAR *ptr;
	int iCurrentSeek;

	if(m_iCurrentBufSize != 0)
	{
		/* Don't insert a new line if this is the first line. */
		InsertNewLine();
	}

	/* Save the current index. The buffer extending function below will
	modify the index on successfull memory allocation. */
	iCurrentSeek = m_iCurrentBufSize;

	ExtendBuffer(lstrlen(LineBuf));
	ptr = &m_Buffer[iCurrentSeek];

	/* Use memcpy instead of strcpy, because do not want to copy
	terminating NULL byte. */
	memcpy(ptr,LineBuf,lstrlen(LineBuf) * sizeof(TCHAR));
	
	return S_OK;
}

HRESULT CBufferManager::InsertNewLine()
{
	HRESULT	BufExtended;
	int		iCurrentSeek;

	/* Save the current index. The buffer extending function below will
	modify the index on successfull memory allocation. */
	iCurrentSeek = m_iCurrentBufSize;

	/* Allocate space for the two-character new line sequence.
	Note that this sequence fits within two bytes, so only
	one wide character is required. */
	BufExtended = ExtendBuffer(1 * sizeof(TCHAR));

	if(BufExtended != S_OK)
		return BufExtended;

	/* Newline sequence under Windows is 0x0D 0x0A. */
	m_Buffer[iCurrentSeek]		= 0x0D;
	m_Buffer[iCurrentSeek + 1]	= 0x0A;

	m_NumLines++;

	return S_OK;
}

/*
Description:	Extends the buffer by the specified amount. All potential memory
				reallocations should come through this function.
*/
HRESULT CBufferManager::ExtendBuffer(int iExtensionSize)
{
	m_Buffer = (TCHAR *)realloc(m_Buffer,(m_iCurrentBufSize + iExtensionSize) * sizeof(TCHAR));

	if(!m_Buffer)
		return E_OUTOFMEMORY;

	/* Need to keep track of how much the buffer size was increased by. */
	m_iCurrentBufSize += iExtensionSize;

	return S_OK;
}

/*
Description:	Flushes the buffer to the specified file. Will
				attempt to create the file if it doesn't already exist.
*/
HRESULT CBufferManager::WriteToFile(TCHAR *FileName)
{
	HANDLE hFile;
	DWORD NumBytesWritten;

	/* Attempt to open the file; if it doesn't exist, create it. */
	hFile = CreateFile(FileName,FILE_WRITE_DATA,0,NULL,
	OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

	if(hFile == INVALID_HANDLE_VALUE)
		return E_FAIL;

	/* Write the buffer to the file... Number of bytes actually written
	will be returned through NumBytesWritten. */
	WriteFile(hFile,(LPCVOID)m_Buffer,m_iCurrentBufSize * sizeof(TCHAR),
		&NumBytesWritten,NULL);

	CloseHandle(hFile);

	/* Full buffer was not properly written. */
	if(NumBytesWritten != (m_iCurrentBufSize * sizeof(TCHAR)))
		return E_UNEXPECTED;

	return S_OK;
}

HRESULT CBufferManager::WriteListEntry(TCHAR *ListEntry)
{
	TCHAR *ptr;
	int iCurrentSeek;

	iCurrentSeek = m_iCurrentBufSize;

	/* Create enough space for the string. */
	ExtendBuffer(lstrlen(ListEntry) + 1);

	ptr = &m_Buffer[iCurrentSeek];
	StringCchCopy(ptr,lstrlen(ListEntry) + 1,ListEntry);

	return S_OK;
}

HRESULT CBufferManager::Write(TCHAR *szBuffer)
{
	TCHAR	*ptr;
	int		iCurrentSeek;

	/* Save the current index. The buffer extending function below will
	modify the index on successful memory allocation. */
	iCurrentSeek = m_iCurrentBufSize;

	ExtendBuffer(lstrlen(szBuffer));
	ptr = &m_Buffer[iCurrentSeek];

	/* Use memcpy instead of strcpy, because do not want to copy
	terminating NULL byte. */
	memcpy(ptr,szBuffer,lstrlen(szBuffer) * sizeof(TCHAR));

	return S_OK;
}

HRESULT CBufferManager::QueryBufferSize(DWORD *BufferSize)
{
	if(BufferSize == NULL)
		return E_INVALIDARG;

	*BufferSize = (UINT)(m_iCurrentBufSize + 1);

	return S_OK;
}

HRESULT CBufferManager::QueryBuffer(TCHAR *DestBuf,UINT DestBufSize)
{
	if(DestBufSize < (UINT)m_iCurrentBufSize)
		return E_FAIL;

	/* Make space for an extra NULL byte (the returned string
	will be double NULL terminated). */
	ExtendBuffer(1);

	/* Add the second terminating NULL byte. */
	m_Buffer[m_iCurrentBufSize - 1] = '\0';

	memcpy(DestBuf,m_Buffer,m_iCurrentBufSize * sizeof(TCHAR));

	return S_OK;
}