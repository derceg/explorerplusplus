// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class ScopedDCState
{
public:
	ScopedDCState(HDC hdc);
	~ScopedDCState();

	bool IsValid() const;

private:
	const HDC m_hdc;
	const int m_savedDc;
};
