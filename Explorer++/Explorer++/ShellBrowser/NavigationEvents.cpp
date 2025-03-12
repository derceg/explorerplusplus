// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "NavigationEvents.h"

boost::signals2::connection NavigationEvents::AddStartedObserver(
	const NavigationSignal::slot_type &observer, const NavigationEventScope &scope,
	boost::signals2::connect_position position, SlotGroup slotGroup)
{
	return m_startedSignal.connect(static_cast<int>(slotGroup),
		MakeFilteredObserver(observer, scope), position);
}

boost::signals2::connection NavigationEvents::AddWillCommitObserver(
	const NavigationSignal::slot_type &observer, const NavigationEventScope &scope,
	boost::signals2::connect_position position, SlotGroup slotGroup)
{
	return m_willCommitSignal.connect(static_cast<int>(slotGroup),
		MakeFilteredObserver(observer, scope), position);
}

boost::signals2::connection NavigationEvents::AddCommittedObserver(
	const NavigationSignal::slot_type &observer, const NavigationEventScope &scope,
	boost::signals2::connect_position position, SlotGroup slotGroup)
{
	return m_committedSignal.connect(static_cast<int>(slotGroup),
		MakeFilteredObserver(observer, scope), position);
}

boost::signals2::connection NavigationEvents::AddFailedObserver(
	const NavigationSignal::slot_type &observer, const NavigationEventScope &scope,
	boost::signals2::connect_position position, SlotGroup slotGroup)
{
	return m_failedSignal.connect(static_cast<int>(slotGroup),
		MakeFilteredObserver(observer, scope), position);
}

boost::signals2::connection NavigationEvents::AddCancelledObserver(
	const NavigationSignal::slot_type &observer, const NavigationEventScope &scope,
	boost::signals2::connect_position position, SlotGroup slotGroup)
{
	return m_cancelledSignal.connect(static_cast<int>(slotGroup),
		MakeFilteredObserver(observer, scope), position);
}

boost::signals2::connection NavigationEvents::AddStoppedObserver(
	const StoppedSignal::slot_type &observer, const NavigationEventScope &scope,
	boost::signals2::connect_position position, SlotGroup slotGroup)
{
	return m_stoppedSignal.connect(static_cast<int>(slotGroup),
		MakeFilteredObserver(observer, scope), position);
}

void NavigationEvents::NotifyStarted(const ShellBrowser *shellBrowser,
	const NavigationRequest *request)
{
	m_startedSignal(shellBrowser, request);
}

void NavigationEvents::NotifyWillCommit(const ShellBrowser *shellBrowser,
	const NavigationRequest *request)
{
	m_willCommitSignal(shellBrowser, request);
}

void NavigationEvents::NotifyCommitted(const ShellBrowser *shellBrowser,
	const NavigationRequest *request)
{
	m_committedSignal(shellBrowser, request);
}

void NavigationEvents::NotifyFailed(const ShellBrowser *shellBrowser,
	const NavigationRequest *request)
{
	m_failedSignal(shellBrowser, request);
}

void NavigationEvents::NotifyCancelled(const ShellBrowser *shellBrowser,
	const NavigationRequest *request)
{
	m_cancelledSignal(shellBrowser, request);
}

void NavigationEvents::NotifyStopped(const ShellBrowser *shellBrowser)
{
	m_stoppedSignal(shellBrowser);
}
