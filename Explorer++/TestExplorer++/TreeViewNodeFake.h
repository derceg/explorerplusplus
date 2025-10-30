// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "TreeViewNode.h"

class TreeViewNodeFake : public TreeViewNode
{
public:
	TreeViewNodeFake(const std::wstring &text = L"");

	std::wstring GetText() const override;
	std::optional<std::wstring> MaybeGetEditingText() const override;
	std::optional<int> GetIconIndex() const override;
	bool CanRename() const override;
	bool CanRemove() const override;
	bool IsGhosted() const override;
	bool IsFile() const override;

	void SetText(const std::wstring &text);
	void SetEditingText(const std::wstring &editingText);
	void ClearEditingText();
	void SetIsGhosted(bool isGhosted);

private:
	std::wstring m_text;
	std::optional<std::wstring> m_editingText;
	bool m_isGhosted = false;
};
