// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AtomicFile.h"
#include <objbase.h>

HRESULT AtomicFile::Create(const std::wstring &destination)
{
	GUID id;
	RETURN_IF_FAILED(CoCreateGuid(&id));
	wchar_t text[40];
	StringFromGUID2(id, text, static_cast<int>(std::size(text)));
	m_destination = destination;
	m_temporaryPath = destination + L".tmp-" + text;
	m_file.reset(CreateFile(m_temporaryPath.c_str(), GENERIC_WRITE | DELETE | FILE_READ_ATTRIBUTES,
		FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
	RETURN_LAST_ERROR_IF(!m_file);
	return S_OK;
}

HRESULT AtomicFile::Write(const void *data, size_t size)
{
	const auto *bytes = static_cast<const char *>(data);
	while (size != 0)
	{
		DWORD written = 0;
		DWORD count = static_cast<DWORD>((std::min) (size, size_t{ 1024 * 1024 }));
		RETURN_IF_WIN32_BOOL_FALSE(WriteFile(m_file.get(), bytes, count, &written, nullptr));
		RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_WRITE_FAULT), written == 0);
		bytes += written;
		size -= written;
	}
	return S_OK;
}

HRESULT AtomicFile::Flush()
{
	RETURN_IF_WIN32_BOOL_FALSE(FlushFileBuffers(m_file.get()));
	return S_OK;
}

HRESULT AtomicFile::Publish(bool replace, const std::wstring &backup)
{
	if (replace)
	{
		// ReplaceFile opens the replacement without sharing. All handles must be
		// closed. Remember its identity so failure cleanup cannot delete a file
		// another process substituted at the temporary path.
		RETURN_IF_WIN32_BOOL_FALSE(GetFileInformationByHandle(m_file.get(), &m_identity));
		m_closedForReplace = true;
		m_file.reset();
		if (ReplaceFile(m_destination.c_str(), m_temporaryPath.c_str(),
				backup.empty() ? nullptr : backup.c_str(), 0, nullptr, nullptr))
		{
			m_accepted = true;
			return S_OK;
		}

		auto error = GetLastError();
		if (error != ERROR_FILE_NOT_FOUND)
		{
			return HRESULT_FROM_WIN32(error);
		}
	}

	// Omitting MOVEFILE_REPLACE_EXISTING also closes the destination-existence race.
	RETURN_IF_WIN32_BOOL_FALSE(
		MoveFileEx(m_temporaryPath.c_str(), m_destination.c_str(), MOVEFILE_WRITE_THROUGH));
	m_published = true;
	if (replace)
		m_accepted = true;
	return S_OK;
}

HRESULT AtomicFile::Discard()
{
	if (m_accepted)
	{
		return S_OK;
	}
	if (!m_file && m_closedForReplace)
	{
		m_file.reset(CreateFile(m_temporaryPath.c_str(), DELETE | FILE_READ_ATTRIBUTES,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
			FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
		if (!m_file)
		{
			auto error = GetLastError();
			return error == ERROR_FILE_NOT_FOUND ? S_OK : HRESULT_FROM_WIN32(error);
		}
		BY_HANDLE_FILE_INFORMATION identity;
		HRESULT error = GetFileInformationByHandle(m_file.get(), &identity)
			? S_OK
			: HRESULT_FROM_WIN32(GetLastError());
		if (SUCCEEDED(error)
			&& (identity.dwVolumeSerialNumber != m_identity.dwVolumeSerialNumber
				|| identity.nFileIndexHigh != m_identity.nFileIndexHigh
				|| identity.nFileIndexLow != m_identity.nFileIndexLow))
			error = HRESULT_FROM_WIN32(ERROR_FILE_INVALID);
		if (FAILED(error))
		{
			m_file.reset();
			return error;
		}
	}
	if (!m_file)
		return S_OK;
	FILE_DISPOSITION_INFO disposition{ TRUE };
	HRESULT hr = SetFileInformationByHandle(m_file.get(), FileDispositionInfo, &disposition,
					 sizeof(disposition))
		? S_OK
		: HRESULT_FROM_WIN32(GetLastError());
	m_file.reset();
	m_closedForReplace = false;
	return hr;
}

AtomicFile::~AtomicFile()
{
	LOG_IF_FAILED(Discard());
}
