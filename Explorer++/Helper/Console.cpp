// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Console.h"
#include <iostream>

bool RedirectConsoleIO();

// The code in this file has been sourced from the following Stack Overflow
// answer:
// https://stackoverflow.com/a/55875595
// It doesn't make much sense to rewrite it, given that it's relatively simple
// and performs a specific function (so any rewritten code would look about the
// same).

bool Console::AttachParentConsole()
{
	bool result = false;

	// Release any current console and redirect IO to NUL
	ReleaseConsole();

	// Attempt to attach to parent process's console
	if (AttachConsole(ATTACH_PARENT_PROCESS))
	{
		result = RedirectConsoleIO();
	}

	return result;
}

bool Console::ReleaseConsole()
{
	bool result = true;
	FILE *fp;

	// Just to be safe, redirect standard IO to NUL before releasing.

	// Redirect STDIN to NUL
	if (freopen_s(&fp, "NUL:", "r", stdin) != 0)
	{
		result = false;
	}
	else
	{
		setvbuf(stdin, NULL, _IONBF, 0);
	}

	// Redirect STDOUT to NUL
	if (freopen_s(&fp, "NUL:", "w", stdout) != 0)
	{
		result = false;
	}
	else
	{
		setvbuf(stdout, NULL, _IONBF, 0);
	}

	// Redirect STDERR to NUL
	if (freopen_s(&fp, "NUL:", "w", stderr) != 0)
	{
		result = false;
	}
	else
	{
		setvbuf(stderr, NULL, _IONBF, 0);
	}

	// Detach from console
	if (!FreeConsole() || !result)
	{
		return false;
	}

	return true;
}

bool RedirectConsoleIO()
{
	bool result = true;
	FILE *fp;

	// Redirect STDIN if the console has an input handle
	if (GetStdHandle(STD_INPUT_HANDLE) != INVALID_HANDLE_VALUE)
	{
		if (freopen_s(&fp, "CONIN$", "r", stdin) != 0)
		{
			result = false;
		}
		else
		{
			setvbuf(stdin, NULL, _IONBF, 0);
		}
	}

	// Redirect STDOUT if the console has an output handle
	if (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE)
	{
		if (freopen_s(&fp, "CONOUT$", "w", stdout) != 0)
		{
			result = false;
		}
		else
		{
			setvbuf(stdout, NULL, _IONBF, 0);
		}
	}

	// Redirect STDERR if the console has an error handle
	if (GetStdHandle(STD_ERROR_HANDLE) != INVALID_HANDLE_VALUE)
	{
		if (freopen_s(&fp, "CONOUT$", "w", stderr) != 0)
		{
			result = false;
		}
		else
		{
			setvbuf(stderr, NULL, _IONBF, 0);
		}
	}

	// Make C++ standard streams point to console as well.
	std::ios::sync_with_stdio(true);

	// Clear the error state for each of the C++ standard streams.
	std::wcout.clear();
	std::cout.clear();
	std::wcerr.clear();
	std::cerr.clear();
	std::wcin.clear();
	std::cin.clear();

	return result;
}
