// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

namespace DarkModeButton
{
	enum class ButtonType
	{
		Checkbox,
		Radio
	};

	// Handles checkboxes and radio buttons.
	void DrawButtonText(const NMCUSTOMDRAW *customDraw, ButtonType buttonType);
}