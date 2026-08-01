// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TerminalHost.h"
#include "PseudoConsoleSession.h"
#include "TerminalControlApi.h"
#include "TerminalOutputParser.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/WindowSubclass.h"
#include <atomic>
#include <climits>
#include <mutex>
#include <optional>
#include <string_view>

namespace
{

struct TerminalScrollState
{
	int viewTop;
	int viewHeight;
	int bufferSize;
};

// TerminalSetTheme doesn't expose a font-only mode. Keep the control's standard Campbell colors
// and blinking bar cursor when changing the font size.
constexpr TerminalTheme DEFAULT_TERMINAL_THEME{ 0x0c0c0c, 0xcccccc, 0xcccccc, 5,
	{ 0x0c0c0c, 0x1f0fc5, 0x0ea113, 0x009cc1, 0xda3700, 0x981788, 0xdd963a, 0xcccccc, 0x767676,
		0x5648e7, 0x0cc616, 0xa5f1f9, 0xff783b, 0x9e00b4, 0xd6d661, 0xf2f2f2 } };
constexpr std::wstring_view TERMINAL_FONT_FAMILY = L"Cascadia Mono";
const UINT TERMINAL_DIRECTORY_CHANGED_MESSAGE =
	RegisterWindowMessage(L"Explorer++TerminalDirectoryChanged");
const UINT TERMINAL_SCROLL_CHANGED_MESSAGE =
	RegisterWindowMessage(L"Explorer++TerminalScrollChanged");
const UINT TERMINAL_PROCESS_EXITED_MESSAGE =
	RegisterWindowMessage(L"Explorer++TerminalProcessExited");

}

class TerminalHost::Impl
{
public:
	static std::unique_ptr<Impl> Create(HWND parent, const TerminalProcessLaunchInfo &launchInfo)
	{
		auto &terminalApi = TerminalControlApi::GetInstance();

		if (!terminalApi.IsAvailable())
		{
			return nullptr;
		}

		auto impl = std::unique_ptr<Impl>(new Impl());
		HRESULT result = terminalApi.createTerminal(parent, &impl->m_window, &impl->m_terminal);

		if (FAILED(result) || !impl->m_window || !impl->m_terminal)
		{
			LOG(WARNING) << "The Windows Terminal control could not be created";
			return nullptr;
		}

		impl->m_windowSubclass = std::make_unique<WindowSubclass>(impl->m_window,
			std::bind_front(&Impl::WindowProcedure, impl.get()));

		LONG_PTR windowStyle = GetWindowLongPtr(impl->m_window, GWL_STYLE);
		SetWindowLongPtr(impl->m_window, GWL_STYLE, windowStyle | WS_VSCROLL);
		SetWindowPos(impl->m_window, nullptr, 0, 0, 0, 0,
			SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

		impl->m_dpi = DpiCompatibility::GetInstance().GetDpiForWindow(parent);
		terminalApi.registerScrollCallback(impl->m_terminal, &Impl::OnTerminalScroll);
		terminalApi.registerWriteCallback(impl->m_terminal, &Impl::OnTerminalWrite);

		{
			ScrollCallbackContext callbackContext(impl.get());
			terminalApi.dpiChanged(impl->m_terminal, impl->m_dpi);
		}

		impl->m_session = PseudoConsoleSession::Create(launchInfo,
			[host = impl.get()](const std::wstring &output) { host->OnOutput(output); });

		if (!impl->m_session)
		{
			return nullptr;
		}

		return impl;
	}

	~Impl()
	{
		m_stopping = true;

		{
			std::scoped_lock lock(s_activeHostMutex);

			if (s_activeHost == this)
			{
				s_activeHost = nullptr;
			}
		}

		m_session.reset();
		m_windowSubclass.reset();

		if (m_terminal)
		{
			auto &terminalApi = TerminalControlApi::GetInstance();
			terminalApi.registerScrollCallback(m_terminal, nullptr);
			terminalApi.destroyTerminal(m_terminal);
		}
	}

	HWND GetHWND() const
	{
		return m_window;
	}

	void SetBounds(int x, int y, int width, int height, bool visible)
	{
		UINT flags = SWP_NOZORDER | SWP_NOACTIVATE | (visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW);
		width = std::max(width, 1);
		height = std::max(height, 1);
		int scrollBarWidth =
			DpiCompatibility::GetInstance().GetSystemMetricsForDpi(SM_CXVSCROLL, m_dpi);
		int rendererWidth = std::max(width - scrollBarWidth, 1);

		TerminalSize dimensions{};
		auto &terminalApi = TerminalControlApi::GetInstance();
		ScrollCallbackContext callbackContext(this);
		HRESULT result = terminalApi.triggerResize(m_terminal, rendererWidth, height, &dimensions);

		// TerminalTriggerResize sizes the renderer and also moves the terminal HWND to (0, 0).
		// Position it within the Explorer++ client area after that internal resize.
		SetWindowPos(m_window, nullptr, x, y, width, height, flags);

		if (SUCCEEDED(result) && dimensions.x > 0 && dimensions.y > 0)
		{
			COORD pseudoConsoleSize{ static_cast<SHORT>(std::min(dimensions.x, SHRT_MAX)),
				static_cast<SHORT>(std::min(dimensions.y, SHRT_MAX)) };
			m_session->Resize(pseudoConsoleSize);
		}

		UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(m_window);

		if (dpi != m_dpi)
		{
			m_dpi = dpi;
			terminalApi.dpiChanged(m_terminal, dpi);
		}
	}

	void SetFontSize(int fontSize)
	{
		short terminalFontSize =
			static_cast<short>(std::clamp(fontSize, 1, static_cast<int>(SHRT_MAX)));

		if (terminalFontSize == m_fontSize)
		{
			return;
		}

		m_fontSize = terminalFontSize;
		ScrollCallbackContext callbackContext(this);
		TerminalControlApi::GetInstance().setTheme(m_terminal, DEFAULT_TERMINAL_THEME,
			TERMINAL_FONT_FAMILY.data(), terminalFontSize, m_dpi);

		if (!m_session)
		{
			return;
		}

		RECT bounds{};
		GetWindowRect(m_window, &bounds);
		MapWindowPoints(nullptr, GetParent(m_window), reinterpret_cast<POINT *>(&bounds), 2);
		SetBounds(bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top,
			IsWindowVisible(m_window));
	}

	void Focus()
	{
		SetActive();
		SetFocus(m_window);
	}

	void SetDirectoryChangedCallback(
		std::function<void(const std::wstring &)> directoryChangedCallback)
	{
		m_directoryChangedCallback = std::move(directoryChangedCallback);
	}

	void SetProcessExitedCallback(std::function<void()> processExitedCallback)
	{
		CHECK(!m_processExitedCallback);
		m_processExitedCallback = std::move(processExitedCallback);
		m_session->SetProcessExitedCallback(
			[this]
			{
				if (!m_stopping)
				{
					PostMessage(m_window, TERMINAL_PROCESS_EXITED_MESSAGE, 0, 0);
				}
			});
	}

private:
	Impl() :
		m_outputParser([this](const std::wstring &directory) { QueueDirectoryChanged(directory); })
	{
	}

	// The native scroll callback doesn't include the originating terminal. Terminal Core invokes it
	// synchronously from API calls, so retain the host for the duration of each such call.
	class ScrollCallbackContext
	{
	public:
		explicit ScrollCallbackContext(Impl *host) : m_previousHost(s_scrollCallbackHost)
		{
			s_scrollCallbackHost = host;
		}

		~ScrollCallbackContext()
		{
			s_scrollCallbackHost = m_previousHost;
		}

	private:
		Impl *m_previousHost;
	};

	void OnOutput(const std::wstring &output)
	{
		if (m_stopping)
		{
			return;
		}

		m_outputParser.Process(output);
		ScrollCallbackContext callbackContext(this);
		TerminalControlApi::GetInstance().sendOutput(m_terminal, output.c_str());
	}

	void QueueDirectoryChanged(const std::wstring &directory)
	{
		bool shouldPostMessage = false;

		{
			std::scoped_lock lock(m_directoryMutex);
			m_pendingDirectory = directory;

			if (!m_directoryNotificationPending)
			{
				m_directoryNotificationPending = true;
				shouldPostMessage = true;
			}
		}

		if (shouldPostMessage && !PostMessage(m_window, TERMINAL_DIRECTORY_CHANGED_MESSAGE, 0, 0))
		{
			std::scoped_lock lock(m_directoryMutex);
			m_directoryNotificationPending = false;
		}
	}

	void NotifyDirectoryChanged()
	{
		std::optional<std::wstring> directory;

		{
			std::scoped_lock lock(m_directoryMutex);
			directory = std::move(m_pendingDirectory);
			m_pendingDirectory.reset();
			m_directoryNotificationPending = false;
		}

		if (directory && m_directoryChangedCallback)
		{
			m_directoryChangedCallback(*directory);
		}
	}

	void NotifyProcessExited()
	{
		if (m_processExitedCallback)
		{
			m_processExitedCallback();
		}
	}

	static void __stdcall OnTerminalScroll(int viewTop, int viewHeight, int bufferSize)
	{
		if (s_scrollCallbackHost)
		{
			s_scrollCallbackHost->QueueScrollChanged({ viewTop, viewHeight, bufferSize });
		}
	}

	void QueueScrollChanged(const TerminalScrollState &scrollState)
	{
		// TerminalSendOutput runs on the pipe reader thread. Marshal scrollbar updates back to the
		// window thread and coalesce them when output arrives rapidly.
		bool shouldPostMessage = false;

		{
			std::scoped_lock lock(m_scrollMutex);
			m_pendingScrollState = scrollState;

			if (!m_scrollNotificationPending)
			{
				m_scrollNotificationPending = true;
				shouldPostMessage = true;
			}
		}

		if (shouldPostMessage && !PostMessage(m_window, TERMINAL_SCROLL_CHANGED_MESSAGE, 0, 0))
		{
			std::scoped_lock lock(m_scrollMutex);
			m_scrollNotificationPending = false;
		}
	}

	void NotifyScrollChanged()
	{
		std::optional<TerminalScrollState> scrollState;

		{
			std::scoped_lock lock(m_scrollMutex);
			scrollState = std::move(m_pendingScrollState);
			m_pendingScrollState.reset();
			m_scrollNotificationPending = false;
		}

		if (!scrollState)
		{
			return;
		}

		SCROLLINFO scrollInfo{ sizeof(scrollInfo) };
		scrollInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;
		scrollInfo.nMin = 0;
		scrollInfo.nMax = std::max(scrollState->bufferSize - 1, 0);
		scrollInfo.nPage = static_cast<UINT>(std::max(scrollState->viewHeight, 0));
		scrollInfo.nPos = std::max(scrollState->viewTop, 0);
		SetScrollInfo(m_window, SB_VERT, &scrollInfo, TRUE);
	}

	void ScrollTo(int position)
	{
		SCROLLINFO scrollInfo{ sizeof(scrollInfo) };
		scrollInfo.fMask = SIF_ALL;

		if (!GetScrollInfo(m_window, SB_VERT, &scrollInfo))
		{
			return;
		}

		int maximumPosition =
			std::max(scrollInfo.nMin, scrollInfo.nMax - static_cast<int>(scrollInfo.nPage) + 1);
		int newPosition = std::clamp(position, scrollInfo.nMin, maximumPosition);

		if (newPosition == scrollInfo.nPos)
		{
			return;
		}

		scrollInfo.fMask = SIF_POS;
		scrollInfo.nPos = newPosition;
		SetScrollInfo(m_window, SB_VERT, &scrollInfo, TRUE);

		ScrollCallbackContext callbackContext(this);
		TerminalControlApi::GetInstance().userScroll(m_terminal, newPosition);
	}

	void OnVerticalScroll(WORD request)
	{
		SCROLLINFO scrollInfo{ sizeof(scrollInfo) };
		scrollInfo.fMask = SIF_ALL;

		if (!GetScrollInfo(m_window, SB_VERT, &scrollInfo))
		{
			return;
		}

		int position = scrollInfo.nPos;

		switch (request)
		{
		case SB_TOP:
			position = scrollInfo.nMin;
			break;

		case SB_BOTTOM:
			position = scrollInfo.nMax;
			break;

		case SB_LINEUP:
			--position;
			break;

		case SB_LINEDOWN:
			++position;
			break;

		case SB_PAGEUP:
			position -= std::max(static_cast<int>(scrollInfo.nPage), 1);
			break;

		case SB_PAGEDOWN:
			position += std::max(static_cast<int>(scrollInfo.nPage), 1);
			break;

		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			position = scrollInfo.nTrackPos;
			break;

		default:
			return;
		}

		ScrollTo(position);
	}

	void OnMouseWheel(WPARAM wParam)
	{
		UINT scrollLines = 0;

		if (!SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &scrollLines, 0) || scrollLines == 0)
		{
			return;
		}

		m_accumulatedWheelDelta += GET_WHEEL_DELTA_WPARAM(wParam);

		SCROLLINFO scrollInfo{ sizeof(scrollInfo) };
		scrollInfo.fMask = SIF_PAGE | SIF_POS;

		if (!GetScrollInfo(m_window, SB_VERT, &scrollInfo))
		{
			return;
		}

		int positionDelta;

		if (scrollLines == WHEEL_PAGESCROLL)
		{
			int pageCount = m_accumulatedWheelDelta / WHEEL_DELTA;
			m_accumulatedWheelDelta %= WHEEL_DELTA;
			positionDelta = -pageCount * std::max(static_cast<int>(scrollInfo.nPage), 1);
		}
		else
		{
			int deltaPerLine = std::max(WHEEL_DELTA / static_cast<int>(scrollLines), 1);
			positionDelta = -m_accumulatedWheelDelta / deltaPerLine;
			m_accumulatedWheelDelta %= deltaPerLine;
		}

		ScrollTo(scrollInfo.nPos + positionDelta);
	}

	static void __stdcall OnTerminalWrite(wchar_t *data)
	{
		std::wstring input;

		if (data)
		{
			input = data;
			CoTaskMemFree(data);
		}

		std::scoped_lock lock(s_activeHostMutex);

		if (s_activeHost)
		{
			s_activeHost->m_session->WriteInput(input);
		}
	}

	void SetActive()
	{
		std::scoped_lock lock(s_activeHostMutex);
		s_activeHost = this;
	}

	LRESULT WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == TERMINAL_DIRECTORY_CHANGED_MESSAGE)
		{
			NotifyDirectoryChanged();
			return 0;
		}

		if (message == TERMINAL_SCROLL_CHANGED_MESSAGE)
		{
			NotifyScrollChanged();
			return 0;
		}

		if (message == TERMINAL_PROCESS_EXITED_MESSAGE)
		{
			NotifyProcessExited();
			return 0;
		}

		auto &terminalApi = TerminalControlApi::GetInstance();
		unsigned short scanCode = static_cast<unsigned short>((lParam >> 16) & 0xff);
		unsigned short flags = static_cast<unsigned short>((lParam >> 16) & 0xff00);

		switch (message)
		{
		case WM_MOUSEACTIVATE:
			SetActive();
			SetFocus(hwnd);
			break;

		case WM_MOUSEWHEEL:
		{
			LRESULT result = DefSubclassProc(hwnd, message, wParam, lParam);
			OnMouseWheel(wParam);
			return result;
		}

		case WM_VSCROLL:
			OnVerticalScroll(LOWORD(wParam));
			return 0;

		case WM_SETFOCUS:
			SetActive();
			terminalApi.setFocus(m_terminal);
			break;

		case WM_KILLFOCUS:
			terminalApi.killFocus(m_terminal);
			break;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			terminalApi.sendKeyEvent(m_terminal, static_cast<unsigned short>(wParam), scanCode,
				flags, true);
			return 0;

		case WM_KEYUP:
		case WM_SYSKEYUP:
			terminalApi.sendKeyEvent(m_terminal, static_cast<unsigned short>(wParam), scanCode,
				flags, false);
			return 0;

		case WM_CHAR:
		case WM_SYSCHAR:
			terminalApi.sendCharEvent(m_terminal, static_cast<wchar_t>(wParam), scanCode, flags);
			return 0;
		}

		return DefSubclassProc(hwnd, message, wParam, lParam);
	}

	static inline std::mutex s_activeHostMutex;
	static inline Impl *s_activeHost = nullptr;
	static inline thread_local Impl *s_scrollCallbackHost = nullptr;

	HWND m_window = nullptr;
	void *m_terminal = nullptr;
	std::unique_ptr<PseudoConsoleSession> m_session;
	std::unique_ptr<WindowSubclass> m_windowSubclass;
	std::atomic_bool m_stopping = false;
	UINT m_dpi = 0;
	short m_fontSize = 0;
	TerminalOutputParser m_outputParser;
	std::mutex m_directoryMutex;
	std::optional<std::wstring> m_pendingDirectory;
	bool m_directoryNotificationPending = false;
	std::function<void(const std::wstring &)> m_directoryChangedCallback;
	std::function<void()> m_processExitedCallback;
	std::mutex m_scrollMutex;
	std::optional<TerminalScrollState> m_pendingScrollState;
	bool m_scrollNotificationPending = false;
	int m_accumulatedWheelDelta = 0;
};

std::unique_ptr<TerminalHost> TerminalHost::Create(HWND parent,
	const TerminalProcessLaunchInfo &launchInfo)
{
	auto impl = Impl::Create(parent, launchInfo);

	if (!impl)
	{
		return nullptr;
	}

	return std::unique_ptr<TerminalHost>(new TerminalHost(std::move(impl)));
}

TerminalHost::TerminalHost(std::unique_ptr<Impl> impl) : m_impl(std::move(impl))
{
}

TerminalHost::~TerminalHost() = default;

HWND TerminalHost::GetHWND() const
{
	return m_impl->GetHWND();
}

void TerminalHost::SetBounds(int x, int y, int width, int height, bool visible)
{
	m_impl->SetBounds(x, y, width, height, visible);
}

void TerminalHost::SetFontSize(int fontSize)
{
	m_impl->SetFontSize(fontSize);
}

void TerminalHost::Focus()
{
	m_impl->Focus();
}

void TerminalHost::SetDirectoryChangedCallback(
	std::function<void(const std::wstring &)> directoryChangedCallback)
{
	m_impl->SetDirectoryChangedCallback(std::move(directoryChangedCallback));
}

void TerminalHost::SetProcessExitedCallback(std::function<void()> processExitedCallback)
{
	m_impl->SetProcessExitedCallback(std::move(processExitedCallback));
}
