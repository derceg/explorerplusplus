// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellContextMenuIdRemapper.h"
#include "ShellContextMenuIdGenerator.h"

ShellContextMenuIdRemapper::ShellContextMenuIdRemapper(ShellContextMenuDelegate *delegate,
	ShellContextMenuIdGenerator *idGenerator) :
	m_delegate(delegate),
	m_idGenerator(idGenerator)
{
}

UINT ShellContextMenuIdRemapper::GenerateUpdatedId(UINT originalId)
{
	// It's valid for the same item to appear in multiple places. For example, the same item might
	// appear in different submenus. When that happens, an updated ID should only be generated once
	// and then reused.
	if (auto existingUpdatedId = MaybeGetUpdateId(originalId))
	{
		return *existingUpdatedId;
	}

	UINT updatedId = m_idGenerator->GetNextId(m_delegate);

	auto [insertionItr, didInsert] = m_updatedIdToOriginalIdMap.insert({ updatedId, originalId });
	DCHECK(didInsert);

	return updatedId;
}

UINT ShellContextMenuIdRemapper::GetOriginalId(UINT updatedId) const
{
	auto itr = m_updatedIdToOriginalIdMap.left.find(updatedId);
	CHECK(itr != m_updatedIdToOriginalIdMap.left.end());
	return itr->second;
}

UINT ShellContextMenuIdRemapper::GetUpdatedId(UINT originalId) const
{
	auto updatedId = MaybeGetUpdateId(originalId);
	CHECK(updatedId);
	return *updatedId;
}

std::optional<UINT> ShellContextMenuIdRemapper::MaybeGetUpdateId(UINT originalId) const
{
	auto itr = m_updatedIdToOriginalIdMap.right.find(originalId);

	if (itr == m_updatedIdToOriginalIdMap.right.end())
	{
		return std::nullopt;
	}

	return itr->second;
}
