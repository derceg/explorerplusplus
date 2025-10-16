// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/core/noncopyable.hpp>
#include <boost/signals2.hpp>
#include <concurrencpp/concurrencpp.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class TreeViewNode;

using TreeViewNodes = std::vector<std::unique_ptr<TreeViewNode>>;

// Acts as a base class for a single node in a TreeView.
class TreeViewNode : private boost::noncopyable
{
public:
	enum class Property
	{
		Text,
		Icon,
		MayLazyLoadChildren
	};

	using UpdatedSignal = boost::signals2::signal<void(Property property)>;

	TreeViewNode();
	virtual ~TreeViewNode() = default;

	int GetId() const;

	virtual std::wstring GetText() const = 0;
	virtual std::optional<int> GetIconIndex() const = 0;
	virtual bool CanRename() const = 0;
	virtual bool CanRemove() const = 0;
	virtual bool IsFile() const = 0;

	// Called only when GetChildren() returns no results. If true, indicates that the node may lazy
	// load children on expansion.
	virtual bool GetMayLazyLoadChildren() const;

	TreeViewNode *GetParent();
	const TreeViewNode *GetParent() const;

	TreeViewNode *AddChild(std::unique_ptr<TreeViewNode> node, size_t index);
	std::unique_ptr<TreeViewNode> RemoveChild(TreeViewNode *node);

	TreeViewNode *GetChildAtIndex(size_t index);
	const TreeViewNode *GetChildAtIndex(size_t index) const;
	size_t GetChildIndex(const TreeViewNode *node) const;
	const TreeViewNodes &GetChildren() const;

	concurrencpp::generator<TreeViewNode *> GetNodesDepthFirst();

	[[nodiscard]] boost::signals2::connection AddUpdatedObserver(
		const typename UpdatedSignal::slot_type &observer);

protected:
	void NotifyUpdated(Property property)
	{
		m_updatedSignal(property);
	}

private:
	static inline int m_idCounter = 1;
	const int m_id;

	TreeViewNode *m_parent = nullptr;
	TreeViewNodes m_children;

	UpdatedSignal m_updatedSignal;
};
