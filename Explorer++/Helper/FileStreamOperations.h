// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <windows.h>
#include <cstdint>
#include <functional>
#include <optional>
#include <stop_token>
#include <string>
#include <vector>

namespace FileStreamOperations
{

// Injectable only at the I/O boundary, so short transfers and disk failures exercise the
// same code as real files. Implementations must not throw.
class Io
{
public:
	virtual ~Io() = default;
	virtual HRESULT Read(HANDLE file, void *buffer, DWORD size, DWORD &transferred);
	virtual HRESULT Write(HANDLE file, const void *buffer, DWORD size, DWORD &transferred);
	virtual HRESULT Flush(HANDLE file);
};

struct Result
{
	HRESULT error = S_OK;
	std::wstring path;
	HRESULT cleanupError = S_OK;
	std::wstring cleanupPath;

	bool Cancelled() const
	{
		return error == HRESULT_FROM_WIN32(ERROR_CANCELLED);
	}
};

using Progress = std::function<void(uint64_t completed, uint64_t total)>;

// No destination is overwritten. On failure/cancellation all outputs owned by this
// operation are removed; any failure of that cleanup is reported separately.
Result Merge(const std::vector<std::wstring> &inputs, const std::wstring &output,
	std::stop_token stop = {}, Progress progress = {}, Io *io = nullptr);
Result Split(const std::wstring &input, const std::wstring &outputDirectory,
	const std::wstring &pattern, uint64_t partSize, std::stop_token stop = {},
	Progress progress = {}, Io *io = nullptr);

std::optional<uint64_t> ParsePartSize(const std::wstring &text, uint64_t multiplier);
bool MoveItem(std::vector<std::wstring> &paths, size_t selected, bool up);
std::wstring DescribeError(const Result &result);

}
