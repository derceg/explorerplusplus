// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/bimap.hpp>
#include <optional>

class ShellContextMenuDelegate;
class ShellContextMenuIdGenerator;

// This class stores the mapping between original IDs and generated IDs for an individual context
// menu delegate. For example, if the delegate adds an item, a unique ID for that item will be
// generated. This class will then store the mapping between the original ID and the generated ID.
class ShellContextMenuIdRemapper
{
public:
	ShellContextMenuIdRemapper(ShellContextMenuDelegate *delegate,
		ShellContextMenuIdGenerator *idGenerator);

	// Generates a unique ID for a single menu item. If an ID has already been generated for the
	// specified item, the existing ID will be returned.
	UINT GenerateUpdatedId(UINT originalId);

	UINT GetOriginalId(UINT updatedId) const;
	UINT GetUpdatedId(UINT originalId) const;

private:
	std::optional<UINT> MaybeGetUpdateId(UINT originalId) const;

	ShellContextMenuDelegate *const m_delegate;
	ShellContextMenuIdGenerator *const m_idGenerator;
	boost::bimap<UINT, UINT> m_updatedIdToOriginalIdMap;
};
