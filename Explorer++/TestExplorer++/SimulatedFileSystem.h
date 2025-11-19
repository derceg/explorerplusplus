// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "SimulatedFileSystemItem.h"
#include "../Helper/PidlHelper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/SignalWrapper.h"
#include <boost/container_hash/hash.hpp>
#include <optional>
#include <string>
#include <unordered_map>

class SimulatedFileSystem
{
public:
	SimulatedFileSystem();

	PidlAbsolute GetRoot();

	PidlAbsolute AddFolder(const PidlAbsolute &parent, const std::wstring &name);
	PidlAbsolute AddFile(const PidlAbsolute &parent, const std::wstring &name);
	PidlAbsolute RenameItem(const PidlAbsolute &pidl, const std::wstring &name,
		std::optional<ShellItemType> itemType = {});
	void NotifyItemUpdated(const PidlAbsolute &pidl);
	void NotifyDirectoryContentsChanged(const PidlAbsolute &pidl);
	void RemoveItem(const PidlAbsolute &pidl);

	PidlAbsolute GetUpdatedPidl(const PidlAbsolute &pidl);
	HRESULT EnumerateFolder(const PidlAbsolute &pidl, std::vector<PidlAbsolute> &output) const;

	// Signals
	SignalWrapper<SimulatedFileSystem, void(const PidlAbsolute &pidl)> itemAddedSignal;
	SignalWrapper<SimulatedFileSystem,
		void(const PidlAbsolute &pidlOld, const PidlAbsolute &pidlNew)>
		itemRenamedSignal;
	SignalWrapper<SimulatedFileSystem, void(const PidlAbsolute &pidl)> itemUpdatedSignal;
	SignalWrapper<SimulatedFileSystem, void(const PidlAbsolute &pidl)>
		directoryContentsChangedSignal;
	SignalWrapper<SimulatedFileSystem, void(const PidlAbsolute &pidl)> itemRemovedSignal;

private:
	static constexpr wchar_t ROOT_FOLDER_PATH[] = L"z:\\dummy";

	PidlAbsolute AddItem(const PidlAbsolute &parent, const std::wstring &name,
		ShellItemType itemType);

	SimulatedFileSystemItem *GetFileSystemItemForPidl(const PidlAbsolute &pidl);
	const SimulatedFileSystemItem *GetFileSystemItemForPidl(const PidlAbsolute &pidl) const;
	const SimulatedFileSystemItem *MaybeGetFileSystemItemForPidl(const PidlAbsolute &pidl) const;

	SimulatedFileSystemItem m_root;
	std::unordered_map<PidlAbsolute, SimulatedFileSystemItem *, boost::hash<PidlAbsolute>>
		m_pidlToItemMap;
};
