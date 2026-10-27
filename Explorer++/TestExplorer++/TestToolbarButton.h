// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ToolbarViewBase.h"

class TestToolbarButton : public ToolbarButton
{
public:
	TestToolbarButton(MouseEventCallback clickedCallback = {});

	// ToolbarButton
	std::wstring GetText() const override;
	std::wstring GetTooltipText() const override;
	std::optional<int> GetImageIndex() const override;
	bool IsEnabled() const override;
	bool IsChecked() const override;

	void SetEnabled(bool enabled);
	void SetChecked(bool checked);

private:
	static inline int m_indexCounter = 0;
	const int m_index;
	bool m_enabled = true;
	bool m_checked = false;
};
