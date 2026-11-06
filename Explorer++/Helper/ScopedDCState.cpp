// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ScopedDCState.h"

ScopedDCState::ScopedDCState(HDC hdc) : m_hdc(hdc), m_savedDc(SaveDC(hdc))
{
}

ScopedDCState::~ScopedDCState()
{
	if (m_savedDc != 0)
	{
		auto res = RestoreDC(m_hdc, m_savedDc);
		DCHECK(res);
	}
}

bool ScopedDCState::IsValid() const
{
	return m_savedDc != 0;
}
