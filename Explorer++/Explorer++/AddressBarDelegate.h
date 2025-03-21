// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

// Allows the AddressBarView controller to be notified of events that occur within the view.
class AddressBarDelegate
{
public:
	virtual ~AddressBarDelegate() = default;

	virtual bool OnKeyPressed(UINT key) = 0;
	virtual void OnBeginDrag() = 0;
};
