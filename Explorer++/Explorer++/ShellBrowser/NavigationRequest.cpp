// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "NavigationRequest.h"
#include "FolderSettings.h"
#include "NavigationEvents.h"
#include "NavigationRequestDelegate.h"
#include "ShellBrowser.h"
#include "ShellEnumerator.h"
#include "../Helper/ShellHelper.h"

NavigationRequest::NavigationRequest(const ShellBrowser *shellBrowser,
	NavigationEvents *navigationEvents, NavigationRequestDelegate *delegate,
	std::shared_ptr<const ShellEnumerator> shellEnumerator,
	std::shared_ptr<concurrencpp::executor> enumerationExecutor,
	std::shared_ptr<concurrencpp::executor> originalExecutor, const NavigateParams &navigateParams,
	std::stop_token stopToken) :
	m_shellBrowser(shellBrowser),
	m_navigationEvents(navigationEvents),
	m_delegate(delegate),
	m_shellEnumerator(shellEnumerator),
	m_enumerationExecutor(enumerationExecutor),
	m_originalExecutor(originalExecutor),
	m_navigateParams(navigateParams),
	m_stopToken(stopToken)
{
}

void NavigationRequest::Start()
{
	SetState(State::Started);

	StartInternal(m_weakPtrFactory.GetWeakPtr());
}

void NavigationRequest::Commit()
{
	SetState(State::WillCommit);
	m_navigationEvents->NotifyWillCommit(this);

	SetState(State::Committed);
	m_navigationEvents->NotifyCommitted(this);

	m_delegate->OnFinished(this);
}

void NavigationRequest::Fail()
{
	SetState(State::Failed);

	m_navigationEvents->NotifyFailed(this);

	m_delegate->OnFinished(this);
}

void NavigationRequest::Cancel()
{
	SetState(State::Cancelled);

	m_navigationEvents->NotifyCancelled(this);

	m_delegate->OnFinished(this);
}

NavigationRequest::State NavigationRequest::GetState() const
{
	return m_state;
}

const NavigateParams &NavigationRequest::GetNavigateParams() const
{
	return m_navigateParams;
}

const ShellBrowser *NavigationRequest::GetShellBrowser() const
{
	return m_shellBrowser;
}

const std::vector<PidlChild> &NavigationRequest::GetItems() const
{
	return m_items;
}

bool NavigationRequest::Stopped() const
{
	return m_stopToken.stop_requested();
}

concurrencpp::null_result NavigationRequest::StartInternal(WeakPtr<NavigationRequest> weakSelf)
{
	// It's not safe to access this object once the coroutine here has switched to a different
	// thread, making it necessary to retrieve these values up front.
	auto shellEnumerator = weakSelf->m_shellEnumerator;
	auto enumerationExecutor = weakSelf->m_enumerationExecutor;
	auto originalExecutor = weakSelf->m_originalExecutor;
	auto navigateParams = weakSelf->m_navigateParams;

	// m_shellBrowser can be null in tests.
	auto *shellBrowser = weakSelf->m_shellBrowser;
	auto showHidden = shellBrowser ? shellBrowser->GetFolderSettings().showHidden : true;

	auto stopToken = weakSelf->m_stopToken;

	weakSelf->m_navigationEvents->NotifyStarted(weakSelf.Get());

	co_await concurrencpp::resume_on(enumerationExecutor);

	// Note that although standard shortcuts (.lnk files) are currently handled outside this class,
	// symlinks and virtual link objects aren't, so they will be handled here.
	//
	// Navigating to the target pidl is important for folders like the quick access folder. Although
	// navigating directly to a recent/pinned folder works as expected, directory monitoring doesn't
	// work. Presumably, that's because directory change notifications are only generated for the
	// original (target) directory. To have things work correctly, the navigation needs to proceed
	// to the original folder instead.
	//
	// Note that this call simply retrieves the target item, but doesn't attempt to resolve it. That
	// matches the behavior of Explorer. For example, if a symlink to a directory is created and the
	// target directory is then removed, Explorer will try to navigate to the target, without
	// attempting to resolve the link.
	unique_pidl_absolute targetPidl;
	HRESULT hr = MaybeGetLinkTarget(navigateParams.pidl.Raw(), targetPidl);

	if (SUCCEEDED(hr))
	{
		navigateParams.pidl = targetPidl.get();
	}

	std::vector<PidlChild> items;
	hr = shellEnumerator->EnumerateDirectory(navigateParams.pidl.Raw(),
		ShellItemFilter::ItemType::FoldersAndFiles,
		showHidden ? ShellItemFilter::HiddenItemPolicy::Include
				   : ShellItemFilter::HiddenItemPolicy::Exclude,
		items, stopToken);

	co_await concurrencpp::resume_on(originalExecutor);

	if (!weakSelf)
	{
		co_return;
	}

	weakSelf->m_navigateParams = navigateParams;
	weakSelf->m_items = items;
	weakSelf->SetState(State::EnumerationFinished);

	if (stopToken.stop_requested())
	{
		weakSelf->m_delegate->OnEnumerationStopped(weakSelf.Get());
		co_return;
	}

	if (FAILED(hr))
	{
		weakSelf->m_delegate->OnEnumerationFailed(weakSelf.Get());
		co_return;
	}

	weakSelf->m_delegate->OnEnumerationCompleted(weakSelf.Get());
}

void NavigationRequest::SetState(State state)
{
	if (state == State::Started)
	{
		CHECK(m_state == State::NotStarted);
	}
	else if (state == State::EnumerationFinished)
	{
		CHECK(m_state == State::Started);
	}
	else if (state == State::WillCommit || state == State::Failed || state == State::Cancelled)
	{
		CHECK(m_state == State::EnumerationFinished);
	}
	else if (state == State::Committed)
	{
		CHECK(m_state == State::WillCommit);
	}
	else
	{
		CHECK(false);
	}

	m_state = state;
}
