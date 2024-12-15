// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/WindowSubclass.h"
#include <boost/core/noncopyable.hpp>
#include <optional>
#include <vector>

struct RebarBandStorageInfo;

class RebarView : private boost::noncopyable
{
public:
	struct Band
	{
		UINT id;
		HWND child;
		UINT height;
		bool newLine;
		bool useChevron;
		bool show;
		UINT length;
		std::optional<UINT> idealLength;
	};

	HWND GetHWND() const;
	UINT GetHeight() const;

	void AddBands(const std::vector<Band> &bands);
	void AddBand(const Band &band);
	void ShowBand(HWND child, bool show);
	void LockBands(bool lock);
	void UpdateBandSize(HWND child, int idealWidth, int height);

	std::vector<RebarBandStorageInfo> GetStorageData() const;

protected:
	RebarView(HWND parent, DWORD style);
	virtual ~RebarView() = default;

private:
	static HWND CreateRebar(HWND parent, DWORD style);

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	const HWND m_hwnd;
	const WindowSubclass m_subclass;
	bool m_locked = false;
};
