// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TestRebarBandView.h"

TestRebarBandView::TestRebarBandView(HWND parent) :
	m_hwnd(CreateWindow(WC_STATIC, L"", WS_CHILD, 0, 0, 0, 0, parent, nullptr,
		GetModuleHandle(nullptr), nullptr))
{
	CHECK(m_hwnd);
}

HWND TestRebarBandView::GetHWND() const
{
	return m_hwnd;
}

int TestRebarBandView::GetHeight() const
{
	return m_height;
}

std::optional<int> TestRebarBandView::MaybeGetIdealWidth() const
{
	return m_idealWidth;
}

void TestRebarBandView::SetHeight(int height)
{
	m_height = height;

	NotifySizeRequirementsChanged();
}

void TestRebarBandView::SetIdealWidth(int idealWidth)
{
	m_idealWidth = idealWidth;

	NotifySizeRequirementsChanged();
}
