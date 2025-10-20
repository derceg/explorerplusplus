// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TreeViewNodeFake.h"

std::wstring TreeViewNodeFake::GetText() const
{
	return L"";
}

std::optional<int> TreeViewNodeFake::GetIconIndex() const
{
	return std::nullopt;
}

bool TreeViewNodeFake::CanRename() const
{
	return true;
}

bool TreeViewNodeFake::CanRemove() const
{
	return true;
}

bool TreeViewNodeFake::IsGhosted() const
{
	return m_isGhosted;
}

bool TreeViewNodeFake::IsFile() const
{
	return false;
}

void TreeViewNodeFake::SetIsGhosted(bool isGhosted)
{
	m_isGhosted = isGhosted;
	NotifyUpdated(Property::Ghosted);
}
