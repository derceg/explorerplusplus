// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ColorRule.h"

ColorRule::ColorRule(const std::wstring &description, const std::wstring &filterPattern,
	bool filterPatternCaseInsensitive, DWORD filterAttributes, COLORREF color) :
	m_description(description),
	m_filterPattern(filterPattern),
	m_filterPatternCaseInsensitive(filterPatternCaseInsensitive),
	m_filterAttributes(filterAttributes),
	m_color(color)
{
}

std::wstring ColorRule::GetDescription() const
{
	return m_description;
}

void ColorRule::SetDescription(const std::wstring &description)
{
	if (description == m_description)
	{
		return;
	}

	m_description = description;

	m_updatedSignal(this);
}

std::wstring ColorRule::GetFilterPattern() const
{
	return m_filterPattern;
}

void ColorRule::SetFilterPattern(const std::wstring &filterPattern)
{
	if (filterPattern == m_filterPattern)
	{
		return;
	}

	m_filterPattern = filterPattern;

	m_updatedSignal(this);
}

bool ColorRule::GetFilterPatternCaseInsensitive() const
{
	return m_filterPatternCaseInsensitive;
}

void ColorRule::SetFilterPatternCaseInsensitive(bool caseInsensitive)
{
	if (caseInsensitive == m_filterPatternCaseInsensitive)
	{
		return;
	}

	m_filterPatternCaseInsensitive = caseInsensitive;

	m_updatedSignal(this);
}

DWORD ColorRule::GetFilterAttributes() const
{
	return m_filterAttributes;
}

void ColorRule::SetFilterAttributes(DWORD attributes)
{
	if (attributes == m_filterAttributes)
	{
		return;
	}

	m_filterAttributes = attributes;

	m_updatedSignal(this);
}

COLORREF ColorRule::GetColor() const
{
	return m_color;
}

void ColorRule::SetColor(COLORREF color)
{
	if (color == m_color)
	{
		return;
	}

	m_color = color;

	m_updatedSignal(this);
}

boost::signals2::connection ColorRule::AddUpdatedObserver(const UpdatedSignal::slot_type &observer)
{
	return m_updatedSignal.connect(observer);
}
