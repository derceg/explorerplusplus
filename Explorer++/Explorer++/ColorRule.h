// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class ColorRule
{
public:
	using UpdatedSignal = boost::signals2::signal<void(ColorRule *colorRule)>;

	ColorRule(const std::wstring &description, const std::wstring &filterPattern,
		bool filterPatternCaseInsensitive, DWORD filterAttributes, COLORREF color);

	std::wstring GetDescription() const;
	void SetDescription(const std::wstring &description);
	std::wstring GetFilterPattern() const;
	void SetFilterPattern(const std::wstring &filterPattern);
	bool GetFilterPatternCaseInsensitive() const;
	void SetFilterPatternCaseInsensitive(bool caseInsensitive);
	DWORD GetFilterAttributes() const;
	void SetFilterAttributes(DWORD attributes);
	COLORREF GetColor() const;
	void SetColor(COLORREF color);

	boost::signals2::connection AddUpdatedObserver(const UpdatedSignal::slot_type &observer);

private:
	std::wstring m_description;
	std::wstring m_filterPattern;
	bool m_filterPatternCaseInsensitive;
	DWORD m_filterAttributes;
	COLORREF m_color;

	UpdatedSignal m_updatedSignal;
};
