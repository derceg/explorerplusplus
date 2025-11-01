// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TreeViewNodeFake.h"

TreeViewNodeFake::TreeViewNodeFake(const std::wstring &text) : m_text(text)
{
}

std::wstring TreeViewNodeFake::GetText() const
{
	return m_text;
}

std::optional<std::wstring> TreeViewNodeFake::MaybeGetEditingText() const
{
	return m_editingText;
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

void TreeViewNodeFake::SetText(const std::wstring &text)
{
	m_text = text;
	NotifyUpdated();
}

void TreeViewNodeFake::SetEditingText(const std::wstring &editingText)
{
	m_editingText = editingText;
}

void TreeViewNodeFake::ClearEditingText()
{
	m_editingText.reset();
}

void TreeViewNodeFake::SetIsGhosted(bool isGhosted)
{
	m_isGhosted = isGhosted;
	NotifyUpdated();
}
