// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class PidlAbsolute;

class PidlUpdater
{
public:
	virtual ~PidlUpdater() = default;

	// Loads an updated version of the pidl. A pidl can contain things like attributes, so updating
	// the pidl can result in an updated set of attributes being loaded, for example.
	virtual PidlAbsolute GetUpdatedPidl(const PidlAbsolute &pidl) = 0;
};
