// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CustomDropHelper.h"
#include "../Helper/Clipboard.h"
#include "../Helper/ClipboardHelper.h"
#include "../Helper/ClipboardStore.h"
#include "../Helper/DataExchangeHelper.h"
#include "../Helper/DataObjectImpl.h"
#include "../Helper/StringHelper.h"
#include <vector>

namespace
{

std::vector<CLIPFORMAT> GetSupportedCustomFormats()
{
	return { CF_UNICODETEXT, static_cast<CLIPFORMAT>(GetPngClipboardFormat()), CF_BITMAP };
}

std::optional<std::string> MaybeExtractText(ClipboardStore *clipboardStore)
{
	Clipboard clipboard(clipboardStore);
	auto text = clipboard.ReadText();

	if (!text)
	{
		return std::nullopt;
	}

	return wstrToUtf8Str(*text);
}

std::optional<std::string> MaybeGetPngData(IWICBitmapSource *bitmap)
{
	wil::com_ptr_nothrow<IStream> stream(SHCreateMemStream(nullptr, 0));

	if (!stream)
	{
		return std::nullopt;
	}

	HRESULT hr = WritePngToStream(bitmap, stream.get());

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	std::string data;
	hr = GetBinaryDataFromIStream(stream.get(), data);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	return data;
}

std::optional<std::string> MaybeExtractPngData(ClipboardStore *clipboardStore)
{
	Clipboard clipboard(clipboardStore);
	auto bitmap = clipboard.ReadPng();

	if (!bitmap)
	{
		return std::nullopt;
	}

	return MaybeGetPngData(bitmap.get());
}

std::optional<std::string> MaybeExtractGdiBitmapData(ClipboardStore *clipboardStore)
{
	Clipboard clipboard(clipboardStore);
	auto bitmap = clipboard.ReadGdiBitmap();

	if (!bitmap)
	{
		return std::nullopt;
	}

	return MaybeGetPngData(bitmap.get());
}

}

namespace CustomDropHelper
{

bool CanPasteInDirectory(ClipboardStore *clipboardStore, PCIDLIST_ABSOLUTE pidl,
	PasteType pasteType)
{
	if (pasteType != PasteType::Normal)
	{
		return false;
	}

	auto customFormats = GetSupportedCustomFormats();
	bool dataAvailable = false;

	for (const auto &customFormat : customFormats)
	{
		if (clipboardStore->IsDataAvailable(customFormat))
		{
			dataAvailable = true;
			break;
		}
	}

	if (!dataAvailable)
	{
		return false;
	}

	// The data extracted from an existing IDataObject instance will always be represented in the
	// same way (via the CFSTR_FILEDESCRIPTOR format). As extracting and converting the data may
	// itself be expensive (e.g. if a large image is on the clipboard) and this function may be
	// called simply to set the state of the "paste" menu item (without a paste being attempted),
	// whether or not the paste will succeed is checked by using a dummy object.
	auto customDataObject = winrt::make<DataObjectImpl>();

	VirtualFile dummyFile(L"Dummy file", "Dummy data");
	HRESULT hr = WriteVirtualFilesToDataObject(customDataObject.get(), { dummyFile });

	if (FAILED(hr))
	{
		return false;
	}

	return CanShellPasteDataObject(pidl, customDataObject.get(), PasteType::Normal);
}

std::optional<FileContents> MaybeExtractData(ClipboardStore *clipboardStore)
{
	if (auto data = MaybeExtractText(clipboardStore))
	{
		return FileContents{ DataType::Text, std::move(*data) };
	}

	if (auto data = MaybeExtractPngData(clipboardStore))
	{
		return FileContents{ DataType::PngImage, std::move(*data) };
	}

	if (auto data = MaybeExtractGdiBitmapData(clipboardStore))
	{
		return FileContents{ DataType::PngImage, std::move(*data) };
	}

	return std::nullopt;
}

}
