// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "TreeViewNode.h"

class TreeViewNodeFake : public TreeViewNode
{
public:
	std::wstring GetText() const override;
	std::optional<int> GetIconIndex() const override;
	bool CanRename() const override;
	bool CanRemove() const override;
	bool IsGhosted() const override;
	bool IsFile() const override;

	void SetIsGhosted(bool isGhosted);

private:
	bool m_isGhosted = false;
};
