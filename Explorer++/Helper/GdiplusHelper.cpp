// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "GdiplusHelper.h"
#include <gdiplus.h>

namespace GdiplusHelper
{

std::optional<CLSID> GetEncoderClsid(const std::wstring &format)
{
	UINT numEncoders;
	UINT size;
	auto res = Gdiplus::GetImageEncodersSize(&numEncoders, &size);

	if (res != Gdiplus::Ok)
	{
		return std::nullopt;
	}

	std::vector<std::byte> rawData(size);
	auto *codecs = reinterpret_cast<Gdiplus::ImageCodecInfo *>(rawData.data());

	res = Gdiplus::GetImageEncoders(numEncoders, size, codecs);

	if (res != Gdiplus::Ok)
	{
		return std::nullopt;
	}

	for (UINT i = 0; i < numEncoders; i++)
	{
		if (codecs[i].MimeType == format)
		{
			return codecs[i].Clsid;
		}
	}

	return std::nullopt;
}

}
