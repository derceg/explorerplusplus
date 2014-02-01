#pragma once

struct CreateFileDeleter
{
	/* The pointer type here is a HANDLE,
	not a HANDLE*. See http://codesequoia.wordpress.com/2012/08/26/stdunique_ptr-for-windows-handles/. */
	typedef HANDLE pointer;

	void operator()(HANDLE handle) const
	{
		if(handle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(handle);
		}
	}
};

struct FindCloseDeleter
{
	typedef HANDLE pointer;

	void operator()(HANDLE handle) const
	{
		if(handle != INVALID_HANDLE_VALUE)
		{
			FindClose(handle);
		}
	}
};