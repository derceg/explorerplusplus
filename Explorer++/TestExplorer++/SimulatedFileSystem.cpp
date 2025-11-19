// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SimulatedFileSystem.h"

SimulatedFileSystem::SimulatedFileSystem() : m_root(ROOT_FOLDER_PATH, ShellItemType::Folder)
{
	m_pidlToItemMap.insert({ m_root.GetPidl(), &m_root });
}

PidlAbsolute SimulatedFileSystem::GetRoot()
{
	return m_root.GetPidl();
}

PidlAbsolute SimulatedFileSystem::AddFolder(const PidlAbsolute &parent, const std::wstring &name)
{
	return AddItem(parent, name, ShellItemType::Folder);
}

PidlAbsolute SimulatedFileSystem::AddFile(const PidlAbsolute &parent, const std::wstring &name)
{
	return AddItem(parent, name, ShellItemType::File);
}

PidlAbsolute SimulatedFileSystem::AddItem(const PidlAbsolute &parent, const std::wstring &name,
	ShellItemType itemType)
{
	auto *parentItem = GetFileSystemItemForPidl(parent);
	CHECK(DoesItemHaveAttributes(parentItem->GetPidl().Raw(), SFGAO_FOLDER));
	auto *item = parentItem->AddChild(std::make_unique<SimulatedFileSystemItem>(name, itemType));

	auto [itr, didInsert] = m_pidlToItemMap.insert({ item->GetPidl(), item });
	CHECK(didInsert);

	itemAddedSignal.m_signal(item->GetPidl());

	return item->GetPidl();
}

PidlAbsolute SimulatedFileSystem::RenameItem(const PidlAbsolute &pidl, const std::wstring &name,
	std::optional<ShellItemType> itemType)
{
	auto *item = GetFileSystemItemForPidl(pidl);
	CHECK(item != &m_root);

	auto pidlOld = item->GetPidl();

	for (const auto *currentItem : item->GetItemsDepthFirst())
	{
		auto numErased = m_pidlToItemMap.erase(currentItem->GetPidl());
		CHECK_EQ(numErased, 1u);
	}

	item->SetName(name, itemType);

	for (auto *currentItem : item->GetItemsDepthFirst())
	{
		auto [itr, didInsert] = m_pidlToItemMap.insert({ currentItem->GetPidl(), currentItem });
		CHECK(didInsert);
	}

	auto pidlNew = item->GetPidl();

	itemRenamedSignal.m_signal(pidlOld, pidlNew);

	return pidlNew;
}

void SimulatedFileSystem::NotifyItemUpdated(const PidlAbsolute &pidl)
{
	itemUpdatedSignal.m_signal(pidl);
}

void SimulatedFileSystem::NotifyDirectoryContentsChanged(const PidlAbsolute &pidl)
{
	CHECK(DoesItemHaveAttributes(pidl.Raw(), SFGAO_FOLDER));
	directoryContentsChangedSignal.m_signal(pidl);
}

void SimulatedFileSystem::RemoveItem(const PidlAbsolute &pidl)
{
	auto *item = GetFileSystemItemForPidl(pidl);
	CHECK(item != &m_root);

	for (const auto *currentItem : item->GetItemsDepthFirst())
	{
		auto numErased = m_pidlToItemMap.erase(currentItem->GetPidl());
		CHECK_EQ(numErased, 1u);
	}

	auto *parent = item->GetParent();
	CHECK(parent);
	auto ownedItem = parent->RemoveChild(item);

	itemRemovedSignal.m_signal(pidl);
}

PidlAbsolute SimulatedFileSystem::GetUpdatedPidl(const PidlAbsolute &pidl)
{
	auto *item = MaybeGetFileSystemItemForPidl(pidl);

	if (!item)
	{
		return pidl;
	}

	return item->GetPidl();
}

HRESULT SimulatedFileSystem::EnumerateFolder(const PidlAbsolute &pidl,
	std::vector<PidlAbsolute> &output) const
{
	auto *item = GetFileSystemItemForPidl(pidl);

	if (!DoesItemHaveAttributes(item->GetPidl().Raw(), SFGAO_FOLDER))
	{
		return E_FAIL;
	}

	for (const auto *child : item->GetChildren())
	{
		output.push_back(child->GetPidl());
	}

	return S_OK;
}

SimulatedFileSystemItem *SimulatedFileSystem::GetFileSystemItemForPidl(const PidlAbsolute &pidl)
{
	return const_cast<SimulatedFileSystemItem *>(
		std::as_const(*this).GetFileSystemItemForPidl(pidl));
}

const SimulatedFileSystemItem *SimulatedFileSystem::GetFileSystemItemForPidl(
	const PidlAbsolute &pidl) const
{
	auto *item = MaybeGetFileSystemItemForPidl(pidl);
	CHECK(item);
	return item;
}

const SimulatedFileSystemItem *SimulatedFileSystem::MaybeGetFileSystemItemForPidl(
	const PidlAbsolute &pidl) const
{
	auto itr = m_pidlToItemMap.find(pidl);

	if (itr == m_pidlToItemMap.end())
	{
		return nullptr;
	}

	return itr->second;
}
