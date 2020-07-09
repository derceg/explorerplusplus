// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DarkModeButton.h"
#include "DarkModeHelper.h"
#include "../Helper/Controls.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/WindowHelper.h"

// Also applies to radio buttons.
constexpr int CHECKBOX_TEXT_SPACING_96DPI = 3;

void DarkModeButton::DrawButtonText(const NMCUSTOMDRAW *customDraw, ButtonType buttonType)
{
	// The size of the interactive element of the control (i.e. the check box or radio button
	// part).
	SIZE elementSize;

	if (buttonType == ButtonType::Checkbox)
	{
		elementSize = GetCheckboxSize(customDraw->hdr.hwndFrom);
	}
	else
	{
		elementSize = GetRadioButtonSize(customDraw->hdr.hwndFrom);
	}

	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(customDraw->hdr.hwndFrom);

	RECT textRect = customDraw->rc;
	textRect.left +=
		elementSize.cx + MulDiv(CHECKBOX_TEXT_SPACING_96DPI, dpi, USER_DEFAULT_SCREEN_DPI);

	std::wstring text = GetWindowString(customDraw->hdr.hwndFrom);
	assert(!text.empty());

	COLORREF textColor;

	if (IsWindowEnabled(customDraw->hdr.hwndFrom))
	{
		textColor = DarkModeHelper::TEXT_COLOR;
	}
	else
	{
		textColor = DarkModeHelper::TEXT_COLOR_DISABLED;
	}

	SetTextColor(customDraw->hdc, textColor);

	UINT textFormat = DT_LEFT;

	if (!WI_IsFlagSet(customDraw->uItemState, CDIS_SHOWKEYBOARDCUES))
	{
		WI_SetFlag(textFormat, DT_HIDEPREFIX);
	}

	RECT finalTextRect = textRect;
	DrawText(customDraw->hdc, text.c_str(), static_cast<int>(text.size()), &finalTextRect,
		textFormat | DT_CALCRECT);

	if (GetRectHeight(&finalTextRect) < GetRectHeight(&textRect))
	{
		textRect.top += (GetRectHeight(&textRect) - GetRectHeight(&finalTextRect)) / 2;
	}

	DrawText(customDraw->hdc, text.c_str(), static_cast<int>(text.size()), &textRect, textFormat);

	if (WI_IsFlagSet(customDraw->uItemState, CDIS_FOCUS))
	{
		DrawFocusRect(customDraw->hdc, &textRect);
	}

	// TODO: May also need to handle CDIS_DISABLED and CDIS_GRAYED.
}