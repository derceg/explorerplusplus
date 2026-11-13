// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <windows.h>
#include <string>

// A new file in the destination directory. Exclusive publication can be rolled back
// until Accept() (needed for a set of split parts). Replacement commits on publication.
// Cleanup uses a handle, verifying file identity if ReplaceFile required closing it.
class AtomicFile
{
public:
	HRESULT Create(const std::wstring &destination);
	const std::wstring &GetCleanupPath() const
	{
		return m_published ? m_destination : m_temporaryPath;
	}
	HRESULT Write(const void *data, size_t size);
	HRESULT Flush();
	HANDLE Get() const
	{
		return m_file.get();
	}
	HRESULT Publish(bool replace = false, const std::wstring &backup = {});
	HRESULT Discard();
	void Accept()
	{
		m_accepted = true;
	}
	~AtomicFile();

private:
	wil::unique_hfile m_file;
	std::wstring m_temporaryPath;
	std::wstring m_destination;
	bool m_accepted = false;
	bool m_published = false;
	bool m_closedForReplace = false;
	BY_HANDLE_FILE_INFORMATION m_identity{};
};
