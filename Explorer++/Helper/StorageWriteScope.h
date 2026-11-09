// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <windows.h>

// The legacy serializers expose void Save methods. A scope collects their first
// write failure, including nested helpers, and prevents committing an incomplete
// snapshot. It is thread-local and restored on scope exit (also on exceptions).
class StorageWriteScope
{
public:
	explicit StorageWriteScope(HRESULT &error, HANDLE transaction = nullptr) :
		m_previousError(s_error),
		m_previousTransaction(s_transaction)
	{
		s_error = &error;
		s_transaction = transaction;
	}
	~StorageWriteScope()
	{
		s_error = m_previousError;
		s_transaction = m_previousTransaction;
	}
	StorageWriteScope(const StorageWriteScope &) = delete;
	StorageWriteScope &operator=(const StorageWriteScope &) = delete;

	static HRESULT Record(HRESULT error)
	{
		if (s_error && FAILED(error) && SUCCEEDED(*s_error))
			*s_error = error;
		return error;
	}
	static LSTATUS RecordWin32(LSTATUS error)
	{
		Record(HRESULT_FROM_WIN32(error));
		return error;
	}
	static HANDLE Transaction()
	{
		return s_transaction;
	}

private:
	inline static thread_local HRESULT *s_error = nullptr;
	inline static thread_local HANDLE s_transaction = nullptr;
	HRESULT *m_previousError;
	HANDLE m_previousTransaction;
};
