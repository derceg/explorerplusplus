// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TerminalTabContent.h"
#include "Config.h"
#include "CustomFont.h"
#include "SystemFontHelper.h"
#include "TerminalClipboard.h"
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

bool IsMessageForWindow(const MSG *msg, HWND window)
{
	return msg->hwnd == window || IsChild(window, msg->hwnd);
}

bool IsPlainControlKeyDown(const MSG *msg, WPARAM key)
{
	return (msg->message == WM_KEYDOWN || msg->message == WM_SYSKEYDOWN) && msg->wParam == key
		&& GetKeyState(VK_CONTROL) < 0 && GetKeyState(VK_SHIFT) >= 0 && GetKeyState(VK_MENU) >= 0;
}

}

std::unique_ptr<TerminalTabContent> TerminalTabContent::Create(HWND parent,
	const TerminalLaunchRequest &launchRequest, const Config *config,
	ClipboardStore *clipboardStore)
{
	auto terminalHost = TerminalHost::Create(parent, launchRequest.process);

	if (!terminalHost)
	{
		return nullptr;
	}

	ShowWindow(terminalHost->GetHWND(), SW_HIDE);
	return std::unique_ptr<TerminalTabContent>(
		new TerminalTabContent(std::move(terminalHost), launchRequest, config, clipboardStore));
}

TerminalTabContent::TerminalTabContent(std::unique_ptr<TerminalHost> terminalHost,
	const TerminalLaunchRequest &launchRequest, const Config *config,
	ClipboardStore *clipboardStore) :
	TabContent(terminalHost->GetHWND()),
	m_terminalHost(std::move(terminalHost)),
	m_terminalClipboard(std::make_unique<TerminalClipboard>(clipboardStore)),
	m_config(config)
{
	if (!launchRequest.initialDirectory.empty())
	{
		m_directory = launchRequest.initialDirectory.wstring();
	}

	m_terminalHost->SetDirectoryChangedCallback(
		std::bind_front(&TerminalTabContent::OnDirectoryChanged, this));

	m_terminalHost->SetProcessExitedCallback(
		std::bind_front(&TerminalTabContent::OnProcessExited, this));

	UpdateFontSize();
	m_mainFontConnection =
		m_config->mainFont.addObserver([this](const auto &) { UpdateFontSize(); });
}

TerminalTabContent::~TerminalTabContent() = default;

void TerminalTabContent::SetBounds(int x, int y, int width, int height, bool visible)
{
	m_terminalHost->SetBounds(x, y, width, height, visible);
}

void TerminalTabContent::Focus()
{
	if (!m_startAttempted)
	{
		m_startAttempted = true;

		if (!m_terminalHost->Start())
		{
			OnProcessExited();
			return;
		}
	}

	m_terminalHost->Focus();
}

std::optional<std::wstring> TerminalTabContent::GetName() const
{
	if (!m_directory)
	{
		return std::nullopt;
	}

	auto path = std::filesystem::path(*m_directory);

	while (!path.has_filename() && path != path.root_path())
	{
		path = path.parent_path();
	}

	auto name = path.filename().wstring();
	return name.empty() ? path.root_path().wstring() : name;
}

std::optional<std::wstring> TerminalTabContent::GetTooltipText() const
{
	return m_directory;
}

std::optional<TabContent::Icon> TerminalTabContent::GetIcon() const
{
	return Icon::CommandLine;
}

TabContent::MessageResult TerminalTabContent::ProcessMessage(const MSG *msg)
{
	if (!IsMessageForWindow(msg, m_terminalHost->GetHWND()))
	{
		return MessageResult::NotHandled;
	}

	if (IsPlainControlKeyDown(msg, 'C') && m_terminalHost->IsSelectionActive())
	{
		auto selection = m_terminalHost->GetSelection();

		if (selection)
		{
			m_terminalClipboard->WriteSelection(*selection);
		}

		return MessageResult::Handled;
	}

	if (IsPlainControlKeyDown(msg, 'V'))
	{
		auto text = m_terminalClipboard->ReadText();

		if (text)
		{
			m_terminalHost->WriteInput(*text);
		}

		return MessageResult::Handled;
	}

	bool isTab = (msg->message == WM_KEYDOWN || msg->message == WM_SYSKEYDOWN)
		&& msg->wParam == VK_TAB && GetKeyState(VK_CONTROL) >= 0 && GetKeyState(VK_MENU) >= 0;

	// With no active selection, Ctrl+C must reach the shell so it retains its interrupt behavior.
	if (isTab || IsPlainControlKeyDown(msg, 'C'))
	{
		return MessageResult::BypassAccelerator;
	}

	return MessageResult::NotHandled;
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

	NotifyUpdated();
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
