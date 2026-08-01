// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PseudoConsoleSession.h"
#include "../Helper/StringHelper.h"
#include <atomic>
#include <bit>
#include <cstddef>
#include <thread>

namespace
{

class PseudoConsoleApi
{
public:
	using CreateFunction = HRESULT(
		WINAPI *)(COORD size, HANDLE input, HANDLE output, DWORD flags, HANDLE *pseudoConsole);
	using ResizeFunction = HRESULT(WINAPI *)(HANDLE pseudoConsole, COORD size);
	using CloseFunction = void(WINAPI *)(HANDLE pseudoConsole);

	static PseudoConsoleApi &GetInstance()
	{
		static PseudoConsoleApi api;
		return api;
	}

	bool IsAvailable() const
	{
		return create != nullptr && resize != nullptr && close != nullptr;
	}

	CreateFunction create = nullptr;
	ResizeFunction resize = nullptr;
	CloseFunction close = nullptr;

private:
	PseudoConsoleApi()
	{
		HMODULE kernel32 = GetModuleHandle(L"kernel32.dll");

		if (!kernel32)
		{
			return;
		}

		create = std::bit_cast<CreateFunction>(GetProcAddress(kernel32, "CreatePseudoConsole"));
		resize = std::bit_cast<ResizeFunction>(GetProcAddress(kernel32, "ResizePseudoConsole"));
		close = std::bit_cast<CloseFunction>(GetProcAddress(kernel32, "ClosePseudoConsole"));
	}
};

constexpr DWORD PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE_VALUE = 0x00020016;
constexpr COORD DEFAULT_TERMINAL_SIZE{ 80, 25 };

}

class PseudoConsoleSession::Impl
{
public:
	static std::unique_ptr<Impl> Create(const TerminalProcessLaunchInfo &launchInfo,
		OutputCallback outputCallback)
	{
		if (!PseudoConsoleApi::GetInstance().IsAvailable())
		{
			return nullptr;
		}

		auto impl = std::unique_ptr<Impl>(new Impl(std::move(outputCallback)));

		if (!impl->Start(launchInfo))
		{
			return nullptr;
		}

		impl->m_outputThread = std::thread(&Impl::ReadOutput, impl.get());
		return impl;
	}

	~Impl()
	{
		m_stopping = true;
		m_inputWrite.reset();

		if (m_pseudoConsole)
		{
			PseudoConsoleApi::GetInstance().close(m_pseudoConsole);
			m_pseudoConsole = nullptr;
		}

		if (m_outputThread.joinable())
		{
			CancelSynchronousIo(m_outputThread.native_handle());
			m_outputThread.join();
		}

		m_outputRead.reset();

		if (m_process && WaitForSingleObject(m_process.get(), 0) == WAIT_TIMEOUT)
		{
			TerminateProcess(m_process.get(), 0);
			WaitForSingleObject(m_process.get(), 1000);
		}

		if (m_processWaitThread.joinable())
		{
			m_processWaitThread.join();
		}
	}

	void Resize(COORD size)
	{
		if (m_pseudoConsole)
		{
			PseudoConsoleApi::GetInstance().resize(m_pseudoConsole, size);
		}
	}

	void WriteInput(const std::wstring &input)
	{
		if (input.empty() || !m_inputWrite)
		{
			return;
		}

		auto utf8 = wstrToUtf8Str(input);
		DWORD bytesWritten = 0;
		WriteFile(m_inputWrite.get(), utf8.data(), static_cast<DWORD>(utf8.size()), &bytesWritten,
			nullptr);
	}

	void SetProcessExitedCallback(std::function<void()> processExitedCallback)
	{
		CHECK(!m_processWaitThread.joinable());
		m_processExitedCallback = std::move(processExitedCallback);
		m_processWaitThread = std::thread(&Impl::WaitForProcessExit, this);
	}

private:
	explicit Impl(OutputCallback outputCallback) : m_outputCallback(std::move(outputCallback))
	{
	}

	bool Start(const TerminalProcessLaunchInfo &launchInfo)
	{
		SECURITY_ATTRIBUTES securityAttributes{ sizeof(securityAttributes), nullptr, TRUE };
		HANDLE pseudoConsoleInputRead = nullptr;
		HANDLE hostInputWrite = nullptr;
		HANDLE hostOutputRead = nullptr;
		HANDLE pseudoConsoleOutputWrite = nullptr;

		if (!CreatePipe(&pseudoConsoleInputRead, &hostInputWrite, &securityAttributes, 0))
		{
			return false;
		}

		wil::unique_handle inputRead(pseudoConsoleInputRead);
		m_inputWrite.reset(hostInputWrite);
		SetHandleInformation(m_inputWrite.get(), HANDLE_FLAG_INHERIT, 0);

		if (!CreatePipe(&hostOutputRead, &pseudoConsoleOutputWrite, &securityAttributes, 0))
		{
			return false;
		}

		m_outputRead.reset(hostOutputRead);
		wil::unique_handle outputWrite(pseudoConsoleOutputWrite);
		SetHandleInformation(m_outputRead.get(), HANDLE_FLAG_INHERIT, 0);

		HRESULT result = PseudoConsoleApi::GetInstance().create(DEFAULT_TERMINAL_SIZE,
			inputRead.get(), outputWrite.get(), 0, &m_pseudoConsole);

		if (FAILED(result))
		{
			LOG(WARNING) << "CreatePseudoConsole failed: " << std::hex << result;
			return false;
		}

		inputRead.reset();
		outputWrite.reset();

		SIZE_T attributeListSize = 0;
		InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeListSize);
		auto attributeListStorage = std::make_unique<std::byte[]>(attributeListSize);
		auto *attributeList =
			reinterpret_cast<PPROC_THREAD_ATTRIBUTE_LIST>(attributeListStorage.get());

		if (!InitializeProcThreadAttributeList(attributeList, 1, 0, &attributeListSize))
		{
			return false;
		}

		auto deleteAttributeList =
			wil::scope_exit([attributeList] { DeleteProcThreadAttributeList(attributeList); });

		if (!UpdateProcThreadAttribute(attributeList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE_VALUE,
				m_pseudoConsole, sizeof(m_pseudoConsole), nullptr, nullptr))
		{
			return false;
		}

		STARTUPINFOEX startupInfo{};
		startupInfo.StartupInfo.cb = sizeof(startupInfo);
		startupInfo.lpAttributeList = attributeList;
		PROCESS_INFORMATION processInformation{};
		auto commandLine = launchInfo.commandLine;

		BOOL created = CreateProcess(launchInfo.application.c_str(), commandLine.data(), nullptr,
			nullptr, FALSE, EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT, nullptr,
			launchInfo.workingDirectory.c_str(), &startupInfo.StartupInfo, &processInformation);

		if (!created)
		{
			LOG(WARNING) << "Could not start the embedded terminal process: " << GetLastError();
			return false;
		}

		m_process.reset(processInformation.hProcess);
		CloseHandle(processInformation.hThread);
		return true;
	}

	void ReadOutput()
	{
		std::string pending;
		char buffer[8192];

		while (!m_stopping)
		{
			DWORD bytesRead = 0;

			if (!ReadFile(m_outputRead.get(), buffer, sizeof(buffer), &bytesRead, nullptr)
				|| bytesRead == 0)
			{
				break;
			}

			pending.append(buffer, bytesRead);
			FlushUtf8(pending, false);
		}

		FlushUtf8(pending, true);
	}

	void WaitForProcessExit()
	{
		WaitForSingleObject(m_process.get(), INFINITE);

		if (!m_stopping && m_processExitedCallback)
		{
			m_processExitedCallback();
		}
	}

	void FlushUtf8(std::string &pending, bool final)
	{
		if (pending.empty() || m_stopping)
		{
			return;
		}

		size_t maximumTail = final ? 0 : std::min<size_t>(3, pending.size());

		for (size_t tail = 0; tail <= maximumTail; ++tail)
		{
			size_t prefixSize = pending.size() - tail;

			if (prefixSize == 0)
			{
				return;
			}

			int requiredSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pending.data(),
				static_cast<int>(prefixSize), nullptr, 0);

			if (requiredSize == 0)
			{
				continue;
			}

			std::wstring output(requiredSize, L'\0');
			MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pending.data(),
				static_cast<int>(prefixSize), output.data(), requiredSize);
			m_outputCallback(output);
			pending.erase(0, prefixSize);
			return;
		}

		// If the stream contains malformed UTF-8, emit replacement characters instead of
		// permanently blocking all subsequent output.
		int requiredSize = MultiByteToWideChar(CP_UTF8, 0, pending.data(),
			static_cast<int>(pending.size()), nullptr, 0);

		if (requiredSize > 0)
		{
			std::wstring output(requiredSize, L'\0');
			MultiByteToWideChar(CP_UTF8, 0, pending.data(), static_cast<int>(pending.size()),
				output.data(), requiredSize);
			m_outputCallback(output);
		}

		pending.clear();
	}

	HANDLE m_pseudoConsole = nullptr;
	wil::unique_handle m_inputWrite;
	wil::unique_handle m_outputRead;
	wil::unique_process_handle m_process;
	std::thread m_outputThread;
	std::thread m_processWaitThread;
	std::atomic_bool m_stopping = false;
	OutputCallback m_outputCallback;
	std::function<void()> m_processExitedCallback;
};

std::unique_ptr<PseudoConsoleSession> PseudoConsoleSession::Create(
	const TerminalProcessLaunchInfo &launchInfo, OutputCallback outputCallback)
{
	auto impl = Impl::Create(launchInfo, std::move(outputCallback));

	if (!impl)
	{
		return nullptr;
	}

	return std::unique_ptr<PseudoConsoleSession>(new PseudoConsoleSession(std::move(impl)));
}

PseudoConsoleSession::PseudoConsoleSession(std::unique_ptr<Impl> impl) : m_impl(std::move(impl))
{
}

PseudoConsoleSession::~PseudoConsoleSession() = default;

void PseudoConsoleSession::Resize(COORD size)
{
	m_impl->Resize(size);
}

void PseudoConsoleSession::WriteInput(const std::wstring &input)
{
	m_impl->WriteInput(input);
}

void PseudoConsoleSession::SetProcessExitedCallback(std::function<void()> processExitedCallback)
{
	m_impl->SetProcessExitedCallback(std::move(processExitedCallback));
}
