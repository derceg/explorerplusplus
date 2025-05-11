// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellContextMenuIdGenerator.h"

ShellContextMenuIdGenerator::ShellContextMenuIdGenerator(UINT startId) : m_nextId(startId)
{
}

UINT ShellContextMenuIdGenerator::GetNextId(ShellContextMenuDelegate *delegate)
{
	UINT id = m_nextId++;
	auto [itr, didInsert] = m_idToDelegateMap.insert({ id, delegate });
	CHECK(didInsert);
	return id;
}

ShellContextMenuDelegate *ShellContextMenuIdGenerator::MaybeGetDelegateForId(UINT id) const
{
	auto itr = m_idToDelegateMap.find(id);

	if (itr == m_idToDelegateMap.end())
	{
		return nullptr;
	}

	return itr->second;
}
