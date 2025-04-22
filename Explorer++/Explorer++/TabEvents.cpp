// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabEvents.h"

boost::signals2::connection TabEvents::AddCreatedObserver(const CreatedSignal::slot_type &observer,
	const TabEventScope &scope, boost::signals2::connect_position position, SlotGroup slotGroup)
{
	return m_createdSignal.connect(static_cast<int>(slotGroup),
		MakeFilteredObserver(observer, scope), position);
}

boost::signals2::connection TabEvents::AddSelectedObserver(
	const SelectedSignal::slot_type &observer, const TabEventScope &scope,
	boost::signals2::connect_position position, SlotGroup slotGroup)
{
	return m_selectedSignal.connect(static_cast<int>(slotGroup),
		MakeFilteredObserver(observer, scope), position);
}

boost::signals2::connection TabEvents::AddMovedObserver(const MovedSignal::slot_type &observer,
	const TabEventScope &scope, boost::signals2::connect_position position, SlotGroup slotGroup)
{
	return m_movedSignal.connect(static_cast<int>(slotGroup), MakeFilteredObserver(observer, scope),
		position);
}

boost::signals2::connection TabEvents::AddPreRemovalObserver(
	const PreRemovalSignal::slot_type &observer, const TabEventScope &scope,
	boost::signals2::connect_position position, SlotGroup slotGroup)
{
	return m_preRemovalSignal.connect(static_cast<int>(slotGroup),
		MakeFilteredObserver(observer, scope), position);
}

boost::signals2::connection TabEvents::AddRemovedObserver(const RemovedSignal::slot_type &observer,
	const TabEventScope &scope, boost::signals2::connect_position position, SlotGroup slotGroup)
{
	return m_removedSignal.connect(static_cast<int>(slotGroup),
		MakeFilteredObserver(observer, scope), position);
}

boost::signals2::connection TabEvents::AddUpdatedObserver(const UpdatedSignal::slot_type &observer,
	const TabEventScope &scope, boost::signals2::connect_position position, SlotGroup slotGroup)
{
	return m_updatedSignal.connect(static_cast<int>(slotGroup),
		MakeFilteredObserver(observer, scope), position);
}

void TabEvents::NotifyCreated(Tab &tab, bool selected)
{
	m_createdSignal(tab, selected);
}

void TabEvents::NotifySelected(const Tab &tab)
{
	m_selectedSignal(tab);
}

void TabEvents::NotifyMoved(const Tab &tab, int fromIndex, int toIndex)
{
	m_movedSignal(tab, fromIndex, toIndex);
}

void TabEvents::NotifyPreRemoval(const Tab &tab, int index)
{
	m_preRemovalSignal(tab, index);
}

void TabEvents::NotifyRemoved(const Tab &tab)
{
	m_removedSignal(tab);
}

void TabEvents::NotifyUpdated(const Tab &tab, Tab::PropertyType propertyType)
{
	m_updatedSignal(tab, propertyType);
}
