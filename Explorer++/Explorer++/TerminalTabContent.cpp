// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TerminalTabContent.h"
#include "Config.h"
#include "CustomFont.h"
#include "SystemFontHelper.h"
#include "TerminalHost.h"
#include "../Helper/DpiCompatibility.h"

namespace
{

int GetMainFontSize(const Config *config)
{
	if (config->mainFont.get())
	{
		return config->mainFont.get()->GetSize();
	}

	auto systemLogFont = GetDefaultSystemFontForDefaultDpi();
	return std::abs(
		DpiCompatibility::GetInstance().PixelsToPointsForDefaultDpi(systemLogFont.lfHeight));
}

}

std::unique_ptr<TerminalTabContent> TerminalTabContent::Create(HWND parent,
	const TerminalLaunchRequest &launchRequest, const Config *config)
{
	auto terminalHost = TerminalHost::Create(parent, launchRequest.process);

	if (!terminalHost)
	{
		return nullptr;
	}

	ShowWindow(terminalHost->GetHWND(), SW_HIDE);
	return std::unique_ptr<TerminalTabContent>(
		new TerminalTabContent(std::move(terminalHost), launchRequest, config));
}

TerminalTabContent::TerminalTabContent(std::unique_ptr<TerminalHost> terminalHost,
	const TerminalLaunchRequest &launchRequest, const Config *config) :
	m_terminalHost(std::move(terminalHost)),
	m_config(config)
{
	if (!launchRequest.initialDirectory.empty())
	{
		m_directory = launchRequest.initialDirectory.wstring();
	}

	m_terminalHost->SetDirectoryChangedCallback(
		std::bind_front(&TerminalTabContent::OnDirectoryChanged, this));

	if (launchRequest.exitBehavior == TerminalExitBehavior::CloseTab)
	{
		m_terminalHost->SetProcessExitedCallback(
			std::bind_front(&TerminalTabContent::OnProcessExited, this));
	}

	UpdateFontSize();
	m_mainFontConnection =
		m_config->mainFont.addObserver([this](const auto &) { UpdateFontSize(); });
}

TerminalTabContent::~TerminalTabContent() = default;

void TerminalTabContent::AttachToTab(HWND replacedWindow)
{
	RECT bounds{};
	GetWindowRect(replacedWindow, &bounds);
	MapWindowPoints(nullptr, GetParent(m_terminalHost->GetHWND()),
		reinterpret_cast<POINT *>(&bounds), 2);

	m_terminalHost->SetBounds(bounds.left, bounds.top, bounds.right - bounds.left,
		bounds.bottom - bounds.top, false);
	ShowWindow(replacedWindow, SW_HIDE);
}

HWND TerminalTabContent::GetHWND() const
{
	return m_terminalHost->GetHWND();
}

void TerminalTabContent::SetBounds(int x, int y, int width, int height, bool visible)
{
	m_terminalHost->SetBounds(x, y, width, height, visible);
}

void TerminalTabContent::Focus()
{
	m_terminalHost->Focus();
}

std::wstring TerminalTabContent::GetName() const
{
	if (!m_directory)
	{
		return {};
	}

	auto path = std::filesystem::path(*m_directory);

	while (!path.has_filename() && path != path.root_path())
	{
		path = path.parent_path();
	}

	auto name = path.filename().wstring();
	return name.empty() ? path.root_path().wstring() : name;
}

std::optional<std::wstring> TerminalTabContent::GetDirectory() const
{
	return m_directory;
}

bool TerminalTabContent::ShouldBypassAccelerator(const MSG *msg) const
{
	if ((msg->message != WM_KEYDOWN && msg->message != WM_SYSKEYDOWN) || msg->wParam != VK_TAB
		|| GetKeyState(VK_CONTROL) < 0 || GetKeyState(VK_MENU) < 0)
	{
		return false;
	}

	HWND terminalWindow = m_terminalHost->GetHWND();
	return msg->hwnd == terminalWindow || IsChild(terminalWindow, msg->hwnd);
}

void TerminalTabContent::SetUpdatedCallback(std::function<void()> updatedCallback)
{
	m_updatedCallback = std::move(updatedCallback);
}

void TerminalTabContent::SetCloseRequestedCallback(std::function<void()> closeRequestedCallback)
{
	m_closeRequestedCallback = std::move(closeRequestedCallback);

	if (m_closeRequestPending)
	{
		m_closeRequestPending = false;
		m_closeRequestedCallback();
	}
}

void TerminalTabContent::UpdateFontSize()
{
	m_terminalHost->SetFontSize(GetMainFontSize(m_config));
}

void TerminalTabContent::OnDirectoryChanged(const std::wstring &directory)
{
	if (directory.empty() || m_directory == directory)
	{
		return;
	}

	m_directory = directory;

	if (m_updatedCallback)
	{
		m_updatedCallback();
	}
}

void TerminalTabContent::OnProcessExited()
{
	if (m_closeRequestedCallback)
	{
		m_closeRequestedCallback();
	}
	else
	{
		m_closeRequestPending = true;
	}
}
