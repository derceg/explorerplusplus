// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RichEditHelper.h"

std::wstring GetRichEditLinkText(const ENLINK *linkNotificationDetails)
{
	std::size_t textLength;

	if (linkNotificationDetails->chrg.cpMax == -1)
	{
		textLength = GetWindowTextLength(linkNotificationDetails->nmhdr.hwndFrom);
	}
	else
	{
		textLength = linkNotificationDetails->chrg.cpMax - linkNotificationDetails->chrg.cpMin;
	}

	std::wstring text(textLength + 1, ' ');

	TEXTRANGE textRange;
	textRange.chrg = linkNotificationDetails->chrg;
	textRange.lpstrText = text.data();
	SendMessage(linkNotificationDetails->nmhdr.hwndFrom, EM_GETTEXTRANGE, 0,
		reinterpret_cast<LPARAM>(&textRange));

	return text;
}