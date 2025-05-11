// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <unordered_map>

class ShellContextMenuDelegate;

// When displaying a shell context menu, the shell items will be assigned a particular ID range.
// This class allows IDs outside of that range to be generated on demand. That ensures that when a
// custom item is added, it can use an ID that's guaranteed not to clash with any other ID (either
// for a shell item or another custom item).
class ShellContextMenuIdGenerator
{
public:
	ShellContextMenuIdGenerator(UINT startId);

	UINT GetNextId(ShellContextMenuDelegate *delegate);
	ShellContextMenuDelegate *MaybeGetDelegateForId(UINT id) const;

private:
	UINT m_nextId;
	std::unordered_map<UINT, ShellContextMenuDelegate *> m_idToDelegateMap;
};
