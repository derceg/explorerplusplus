// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DataExchangeHelper.h"
#include "DragDropHelper.h"
#include "GraphicsFactories.h"
#include "Helper.h"
#include "ImageHelper.h"
#include "UniqueVariableSizeStruct.h"
#include <wil/com.h>

std::optional<std::wstring> ReadStringFromGlobal(HGLOBAL global)
{
	wil::unique_hglobal_locked mem(global);

	if (!mem)
	{
		return std::nullopt;
	}

	auto size = GlobalSize(global);

	if (size == 0 || (size % sizeof(wchar_t)) != 0)
	{
		return std::nullopt;
	}

	auto text = std::wstring(static_cast<const wchar_t *>(mem.get()), size / sizeof(wchar_t));

	auto nullPos = text.find(L'\0');

	if (nullPos != std::wstring::npos)
	{
		text.resize(nullPos);
	}

	return text;
}

wil::unique_hglobal WriteStringToGlobal(const std::wstring &str)
{
	return WriteDataToGlobal(str.c_str(), (str.size() + 1) * sizeof(wchar_t));
}

std::optional<std::string> ReadBinaryDataFromGlobal(HGLOBAL global)
{
	wil::unique_hglobal_locked mem(global);

	if (!mem)
	{
		return std::nullopt;
	}

	auto size = GlobalSize(global);

	if (size == 0)
	{
		return std::nullopt;
	}

	return std::string(static_cast<const char *>(mem.get()), size);
}

wil::unique_hglobal WriteBinaryDataToGlobal(const std::string &data)
{
	return WriteDataToGlobal(data.data(), data.size());
}

wil::unique_hglobal WriteDataToGlobal(const void *data, size_t size)
{
	wil::unique_hglobal global(GlobalAlloc(GMEM_MOVEABLE, size));

	if (!global)
	{
		return nullptr;
	}

	wil::unique_hglobal_locked mem(global.get());

	if (!mem)
	{
		return nullptr;
	}

	memcpy(mem.get(), data, size);

	return global;
}

std::optional<std::vector<std::wstring>> ReadHDropDataFromGlobal(HGLOBAL global)
{
	wil::unique_hglobal_locked mem(global);

	if (!mem)
	{
		return std::nullopt;
	}

	auto dropData = static_cast<HDROP>(mem.get());
	std::vector<std::wstring> paths;

	UINT numFiles = DragQueryFile(dropData, 0xFFFFFFFF, nullptr, 0);

	for (UINT i = 0; i < numFiles; i++)
	{
		UINT numCharacters = DragQueryFile(dropData, i, nullptr, 0);

		if (numCharacters == 0)
		{
			continue;
		}

		// The return value of DragQueryFile() doesn't include space for the terminating null
		// character.
		std::wstring path;
		path.resize(numCharacters + 1);

		numCharacters = DragQueryFile(dropData, i, path.data(), static_cast<UINT>(path.size()));

		if (numCharacters == 0)
		{
			continue;
		}

		path.resize(numCharacters);

		paths.push_back(path);
	}

	if (paths.empty())
	{
		return std::nullopt;
	}

	return paths;
}

wil::unique_hglobal WriteHDropDataToGlobal(const std::vector<std::wstring> &paths)
{
	if (paths.empty())
	{
		// An empty list of filenames isn't valid.
		return nullptr;
	}

	std::wstring concatenatedPaths;

	for (const auto &path : paths)
	{
		concatenatedPaths.append(path);
		concatenatedPaths.append(1, '\0');
	}

	// The list of filenames needs to be double null-terminated.
	concatenatedPaths.append(1, '\0');

	size_t headerSize = sizeof(DROPFILES);
	size_t concatenatedPathsSize = concatenatedPaths.size() * sizeof(wchar_t);

	wil::unique_hglobal global(GlobalAlloc(GHND, headerSize + concatenatedPathsSize));

	if (!global)
	{
		return nullptr;
	}

	wil::unique_hglobal_locked mem(global.get());

	if (!mem)
	{
		return nullptr;
	}

	auto dropData = static_cast<DROPFILES *>(mem.get());
	dropData->pFiles = static_cast<DWORD>(headerSize);
	dropData->fWide = true;

	auto *filenameData = reinterpret_cast<std::byte *>(dropData) + headerSize;
	std::memcpy(filenameData, concatenatedPaths.data(), concatenatedPathsSize);

	return global;
}

wil::com_ptr_nothrow<IWICBitmapSource> ReadPngDataFromGlobal(HGLOBAL global)
{
	wil::com_ptr_nothrow<IStream> stream;
	HRESULT hr = CreateStreamOnHGlobal(global, false, &stream);

	if (FAILED(hr))
	{
		return nullptr;
	}

	wil::com_ptr_nothrow<IWICBitmapSource> bitmap;
	hr = ImageHelper::LoadBitmapFromStream(stream.get(), bitmap);

	if (FAILED(hr))
	{
		return nullptr;
	}

	return bitmap;
}

wil::unique_hglobal WritePngDataToGlobal(IWICBitmapSource *bitmap)
{
	wil::com_ptr_nothrow<IStream> stream;
	HRESULT hr = CreateStreamOnHGlobal(nullptr, false, &stream);

	if (FAILED(hr))
	{
		return nullptr;
	}

	// As noted in https://devblogs.microsoft.com/oldnewthing/20210929-00/?p=105742, if this
	// function were to fail, the global inside the stream would be leaked. However, that post also
	// notes that this function is guaranteed to succeed if the stream it's given came from
	// CreateStreamOnHGlobal(), hence the CHECK().
	wil::unique_hglobal global;
	hr = GetHGlobalFromStream(stream.get(), &global);
	CHECK(SUCCEEDED(hr));

	WritePngToStream(bitmap, stream.get());

	return global;
}

HRESULT WritePngToStream(IWICBitmapSource *bitmap, IStream *stream)
{
	auto *imagingFactory = GraphicsFactories::GetWICFactory();

	wil::com_ptr_nothrow<IWICBitmapEncoder> encoder;
	RETURN_IF_FAILED(imagingFactory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder));
	RETURN_IF_FAILED(encoder->Initialize(stream, WICBitmapEncoderNoCache));

	wil::com_ptr_nothrow<IWICBitmapFrameEncode> frame;
	RETURN_IF_FAILED(encoder->CreateNewFrame(&frame, nullptr));
	RETURN_IF_FAILED(frame->Initialize(nullptr));
	RETURN_IF_FAILED(frame->WriteSource(bitmap, nullptr));
	RETURN_IF_FAILED(frame->Commit());
	RETURN_IF_FAILED(encoder->Commit());

	return S_OK;
}

HRESULT ReadVirtualFilesFromDataObject(IDataObject *dataObject,
	std::vector<VirtualFile> &virtualFilesOutput)
{
	UniqueVariableSizeStruct<FILEGROUPDESCRIPTOR> fileGroupDescriptor;
	RETURN_IF_FAILED(GetBlobData(dataObject,
		static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR)),
		fileGroupDescriptor));

	std::vector<VirtualFile> virtualFiles;

	for (UINT i = 0; i < fileGroupDescriptor->cItems; i++)
	{
		if (WI_IsFlagSet(fileGroupDescriptor->fgd[i].dwFlags, FD_ATTRIBUTES)
			&& WI_IsFlagSet(fileGroupDescriptor->fgd[i].dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
		{
			continue;
		}

		FORMATETC contentsFormat = { static_cast<CLIPFORMAT>(
										 RegisterClipboardFormat(CFSTR_FILECONTENTS)),
			nullptr, DVASPECT_CONTENT, CheckedNumericCast<LONG>(i), TYMED_HGLOBAL };
		std::string contents;
		RETURN_IF_FAILED(GetBlobData(dataObject, &contentsFormat, contents));

		virtualFiles.emplace_back(fileGroupDescriptor->fgd[i].cFileName, contents);
	}

	if (virtualFiles.empty())
	{
		return E_FAIL;
	}

	virtualFilesOutput = virtualFiles;

	return S_OK;
}

HRESULT WriteVirtualFilesToDataObject(IDataObject *dataObject,
	const std::vector<VirtualFile> &virtualFiles)
{
	if (virtualFiles.empty())
	{
		return E_FAIL;
	}

	size_t groupDescriptorSize =
		sizeof(FILEGROUPDESCRIPTOR) + (sizeof(FILEDESCRIPTOR) * virtualFiles.size() - 1);
	auto groupDescriptor = MakeUniqueVariableSizeStruct<FILEGROUPDESCRIPTOR>(groupDescriptorSize);

	groupDescriptor->cItems = CheckedNumericCast<UINT>(virtualFiles.size());

	for (size_t i = 0; i < virtualFiles.size(); i++)
	{
		ULARGE_INTEGER fileSize;
		fileSize.QuadPart = virtualFiles[i].contents.size();

		FILEDESCRIPTOR fileDescriptor = {};
		fileDescriptor.nFileSizeLow = fileSize.LowPart;
		fileDescriptor.nFileSizeHigh = fileSize.HighPart;
		fileDescriptor.dwFlags = static_cast<DWORD>(FD_UNICODE) | FD_FILESIZE;
		RETURN_IF_FAILED(StringCchCopy(fileDescriptor.cFileName,
			std::size(fileDescriptor.cFileName), virtualFiles[i].name.c_str()));

		groupDescriptor->fgd[i] = fileDescriptor;

		FORMATETC contentsFormat = { static_cast<CLIPFORMAT>(
										 RegisterClipboardFormat(CFSTR_FILECONTENTS)),
			nullptr, DVASPECT_CONTENT, CheckedNumericCast<LONG>(i), TYMED_HGLOBAL };
		RETURN_IF_FAILED(SetBlobData(dataObject, &contentsFormat, virtualFiles[i].contents.data(),
			virtualFiles[i].contents.size()));
	}

	RETURN_IF_FAILED(SetBlobData(dataObject,
		static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR)),
		groupDescriptor.get(), groupDescriptorSize));

	return S_OK;
}

bool IsDropFormatAvailable(IDataObject *dataObject, const FORMATETC &formatEtc)
{
	FORMATETC formatEtcCopy = formatEtc;
	HRESULT hr = dataObject->QueryGetData(&formatEtcCopy);
	return (hr == S_OK);
}

UINT GetPngClipboardFormat()
{
	// This is used by applications like Chrome when copying an image. The clipboard will contain
	// the raw png data.
	static UINT clipboardFormat = RegisterClipboardFormat(L"PNG");
	return clipboardFormat;
}

HRESULT MoveStorageToObject(IDataObject *dataObject, FORMATETC *format, wil::unique_stg_medium stg)
{
	RETURN_IF_FAILED(dataObject->SetData(format, &stg, true));

	// The IDataObject instance now owns the STGMEDIUM structure and is responsible for freeing the
	// memory associated with it.
	stg.release();

	return S_OK;
}

wil::unique_hglobal CloneGlobal(HGLOBAL global)
{
	wil::unique_hglobal_locked sourceMemory(global);

	if (!sourceMemory)
	{
		return nullptr;
	}

	auto size = GlobalSize(global);

	if (size == 0)
	{
		return nullptr;
	}

	wil::unique_hglobal clone(GlobalAlloc(GMEM_MOVEABLE, size));

	if (!clone)
	{
		return nullptr;
	}

	wil::unique_hglobal_locked destinationMemory(clone.get());

	if (!destinationMemory)
	{
		return nullptr;
	}

	memcpy(destinationMemory.get(), sourceMemory.get(), size);

	return clone;
}

HRESULT GetBinaryDataFromIStream(IStream *stream, std::string &output)
{
	STATSTG stat;
	RETURN_IF_FAILED(stream->Stat(&stat, STATFLAG_NONAME));

	LARGE_INTEGER offset = { 0 };
	RETURN_IF_FAILED(stream->Seek(offset, STREAM_SEEK_SET, nullptr));

	if (stat.cbSize.QuadPart > std::numeric_limits<ULONG>::max())
	{
		return E_FAIL;
	}

	auto size = static_cast<ULONG>(stat.cbSize.QuadPart);

	std::string result;
	result.resize(size);

	ULONG bytesRead = 0;
	RETURN_IF_FAILED(stream->Read(result.data(), size, &bytesRead));

	if (bytesRead != size)
	{
		return E_FAIL;
	}

	output = std::move(result);

	return S_OK;
}
