// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FileStreamOperations.h"
#include "AtomicFile.h"
#include <comdef.h>
#include <filesystem>
#include <limits>
#include <memory>

namespace FileStreamOperations
{
namespace
{
constexpr DWORD BUFFER_SIZE = 1024 * 1024;

// Cancellation is registered only around cancellable I/O. Unregistering waits
// for an in-flight stop callback, so a late stop cannot interrupt rollback.
class IoCancellation
{
public:
	HRESULT Initialize(std::stop_token stop)
	{
		if (!stop.stop_possible())
			return S_OK;
		RETURN_IF_WIN32_BOOL_FALSE(DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
			GetCurrentProcess(), m_thread.put(), 0, FALSE, DUPLICATE_SAME_ACCESS));
		m_callback.emplace(stop, Cancel{ m_thread.get() });
		return S_OK;
	}

private:
	struct Cancel
	{
		HANDLE thread;
		void operator()() const
		{
			CancelSynchronousIo(thread);
		}
	};
	wil::unique_handle m_thread;
	std::optional<std::stop_callback<Cancel>> m_callback;
};

HRESULT Flush(HANDLE file, std::stop_token stop, Io &io)
{
	IoCancellation cancellation;
	RETURN_IF_FAILED(cancellation.Initialize(stop));
	if (stop.stop_requested())
		return HRESULT_FROM_WIN32(ERROR_CANCELLED);
	return io.Flush(file);
}

HRESULT Copy(HANDLE input, HANDLE output, uint64_t size, std::stop_token stop, Io &io,
	std::vector<char> &buffer, bool &writing)
{
	IoCancellation cancellation;
	RETURN_IF_FAILED(cancellation.Initialize(stop));
	while (size != 0)
	{
		if (stop.stop_requested())
		{
			return HRESULT_FROM_WIN32(ERROR_CANCELLED);
		}
		DWORD read = 0;
		writing = false;
		RETURN_IF_FAILED(io.Read(input, buffer.data(),
			static_cast<DWORD>((std::min) (size, static_cast<uint64_t>(buffer.size()))), read));
		if (read == 0)
		{
			return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
		}
		if (read > (std::min) (size, static_cast<uint64_t>(buffer.size())))
		{
			return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
		}
		DWORD offset = 0;
		while (offset < read)
		{
			if (stop.stop_requested())
			{
				return HRESULT_FROM_WIN32(ERROR_CANCELLED);
			}
			DWORD written = 0;
			writing = true;
			RETURN_IF_FAILED(io.Write(output, buffer.data() + offset, read - offset, written));
			if (written == 0 || written > read - offset)
			{
				return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
			}
			offset += written;
		}
		size -= read;
	}
	return S_OK;
}

HRESULT OpenInput(const std::wstring &path, wil::unique_hfile &file, uint64_t &size,
	std::stop_token stop)
{
	IoCancellation cancellation;
	RETURN_IF_FAILED(cancellation.Initialize(stop));
	if (stop.stop_requested())
		return HRESULT_FROM_WIN32(ERROR_CANCELLED);
	// Deny modifications/removal for the duration of each copy.
	file.reset(CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN, nullptr));
	RETURN_LAST_ERROR_IF(!file);
	LARGE_INTEGER length;
	RETURN_IF_WIN32_BOOL_FALSE(GetFileSizeEx(file.get(), &length));
	size = static_cast<uint64_t>(length.QuadPart);
	return S_OK;
}

void Cleanup(AtomicFile &file, Result &result)
{
	HRESULT hr = file.Discard();
	if (FAILED(hr) && SUCCEEDED(result.cleanupError))
	{
		result.cleanupError = hr;
		result.cleanupPath = file.GetCleanupPath();
	}
}
}

HRESULT Io::Read(HANDLE file, void *buffer, DWORD size, DWORD &transferred)
{
	return ReadFile(file, buffer, size, &transferred, nullptr) ? S_OK
															   : HRESULT_FROM_WIN32(GetLastError());
}

HRESULT Io::Write(HANDLE file, const void *buffer, DWORD size, DWORD &transferred)
{
	return WriteFile(file, buffer, size, &transferred, nullptr)
		? S_OK
		: HRESULT_FROM_WIN32(GetLastError());
}

HRESULT Io::Flush(HANDLE file)
{
	return FlushFileBuffers(file) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}

Result Merge(const std::vector<std::wstring> &inputs, const std::wstring &output,
	std::stop_token stop, Progress progress, Io *providedIo)
{
	Io defaultIo;
	Io &io = providedIo ? *providedIo : defaultIo;
	AtomicFile destination;
	Result result{ S_OK, output };
	try
	{
		result.error = destination.Create(output);
		std::vector<char> buffer(BUFFER_SIZE);
		uint64_t completed = 0;
		if (inputs.empty())
		{
			result.error = E_INVALIDARG;
		}
		for (const auto &path : inputs)
		{
			if (FAILED(result.error))
				break;
			result.path = path;
			wil::unique_hfile input;
			uint64_t size = 0;
			result.error = OpenInput(path, input, size, stop);
			if (SUCCEEDED(result.error))
			{
				bool writing = false;
				result.error =
					Copy(input.get(), destination.Get(), size, stop, io, buffer, writing);
				if (FAILED(result.error) && writing)
					result.path = output;
			}
			if (FAILED(result.error))
				break;
			if (progress)
				progress(++completed, inputs.size());
		}
		if (SUCCEEDED(result.error))
		{
			result.path = output;
			result.error = stop.stop_requested() ? HRESULT_FROM_WIN32(ERROR_CANCELLED)
												 : Flush(destination.Get(), stop, io);
		}
		if (SUCCEEDED(result.error) && stop.stop_requested())
			result.error = HRESULT_FROM_WIN32(ERROR_CANCELLED);
		if (SUCCEEDED(result.error))
			result.error = destination.Publish();
		if (SUCCEEDED(result.error))
			destination.Accept();
	}
	catch (const std::bad_alloc &)
	{
		result.error = E_OUTOFMEMORY;
	}
	Cleanup(destination, result);
	if (stop.stop_requested() && result.error == HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED))
		result.error = HRESULT_FROM_WIN32(ERROR_CANCELLED);
	return result;
}

Result Split(const std::wstring &inputPath, const std::wstring &outputDirectory,
	const std::wstring &pattern, uint64_t partSize, std::stop_token stop, Progress progress,
	Io *providedIo)
{
	const auto counter = pattern.find(L"/N");
	// The pattern is a filename, not a relative path. /N is the sole allowed separator.
	auto name = pattern;
	if (counter != std::wstring::npos)
		name.erase(counter, 2);
	if (partSize == 0 || counter == std::wstring::npos
		|| name.find_first_of(L"/\\:") != std::wstring::npos)
	{
		return { E_INVALIDARG, pattern };
	}
	Io defaultIo;
	Io &io = providedIo ? *providedIo : defaultIo;
	struct Part
	{
		std::wstring path;
		std::unique_ptr<AtomicFile> file;
	};
	std::vector<Part> parts;
	Result result{ S_OK, inputPath };
	try
	{
		wil::unique_hfile input;
		uint64_t size = 0;
		result.error = OpenInput(inputPath, input, size, stop);
		if (FAILED(result.error))
			return result;
		uint64_t total = size / partSize + (size % partSize != 0);
		std::vector<char> buffer(BUFFER_SIZE);
		uint64_t remaining = size;
		for (uint64_t index = 1; index <= total; ++index)
		{
			if (stop.stop_requested())
			{
				result.error = HRESULT_FROM_WIN32(ERROR_CANCELLED);
				break;
			}
			auto filename = pattern;
			filename.replace(counter, 2, std::to_wstring(index));
			auto path = (std::filesystem::path(outputDirectory) / filename).wstring();
			result.path = path;
			parts.push_back({ path, std::make_unique<AtomicFile>() });
			auto &part = *parts.back().file;
			result.error = part.Create(path);
			if (FAILED(result.error))
				break;
			auto length = (std::min) (remaining, partSize);
			bool writing = false;
			result.error = Copy(input.get(), part.Get(), length, stop, io, buffer, writing);
			if (FAILED(result.error) && !writing)
				result.path = inputPath;
			if (FAILED(result.error))
				break;
			result.error = Flush(part.Get(), stop, io);
			if (FAILED(result.error))
				break;
			remaining -= length;
			if (progress)
				progress(index, total);
		}
		if (SUCCEEDED(result.error) && stop.stop_requested())
		{
			result.error = HRESULT_FROM_WIN32(ERROR_CANCELLED);
		}
		if (SUCCEEDED(result.error))
		{
			for (auto &part : parts)
			{
				result.path = part.path;
				if (stop.stop_requested())
				{
					result.error = HRESULT_FROM_WIN32(ERROR_CANCELLED);
					break;
				}
				result.error = part.file->Publish();
				if (FAILED(result.error))
					break;
			}
		}
		if (SUCCEEDED(result.error))
		{
			for (auto &part : parts)
				part.file->Accept();
		}
	}
	catch (const std::bad_alloc &)
	{
		result.error = E_OUTOFMEMORY;
	}
	for (auto &part : parts)
		Cleanup(*part.file, result);
	if (stop.stop_requested() && result.error == HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED))
		result.error = HRESULT_FROM_WIN32(ERROR_CANCELLED);
	return result;
}

std::optional<uint64_t> ParsePartSize(const std::wstring &text, uint64_t multiplier)
{
	if (text.empty() || multiplier == 0)
		return std::nullopt;
	uint64_t value = 0;
	for (auto character : text)
	{
		if (character < L'0' || character > L'9')
			return std::nullopt;
		const auto digit = static_cast<unsigned>(character - L'0');
		if (value > ((std::numeric_limits<uint64_t>::max)() - digit) / 10)
			return std::nullopt;
		value = value * 10 + digit;
	}
	const auto maximum = static_cast<uint64_t>((std::numeric_limits<int64_t>::max)());
	if (value == 0 || value > maximum / multiplier)
		return std::nullopt;
	return value * multiplier;
}

bool MoveItem(std::vector<std::wstring> &paths, size_t selected, bool up)
{
	if (selected >= paths.size() || (up && selected == 0) || (!up && selected + 1 == paths.size()))
		return false;
	std::swap(paths[selected], paths[up ? selected - 1 : selected + 1]);
	return true;
}

std::wstring DescribeError(const Result &result)
{
	std::wstring message = result.path + L"\n" + _com_error(result.error).ErrorMessage();
	if (FAILED(result.cleanupError))
	{
		message +=
			L"\n" + result.cleanupPath + L"\n" + _com_error(result.cleanupError).ErrorMessage();
	}
	return message;
}
}
