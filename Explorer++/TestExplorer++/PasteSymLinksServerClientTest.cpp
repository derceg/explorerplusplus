// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "PasteSymLinksClient.h"
#include "PasteSymLinksServer.h"
#include <gtest/gtest.h>

using namespace std::chrono_literals;

TEST(PasteSymLinksServerClientTest, ClientSendsResultsNormally)
{
	PasteSymLinksServer server;

	ClipboardOperations::PastedItems pastedItems = {
		{ L"C:\\file1", {} },
		{ L"C:\\file2", { ERROR_ACCESS_DENIED, std::system_category() } },
		{ L"C:\\file3", {} },
	};

	std::jthread thread;

	auto clientThreadBody = [&pastedItems]
	{
		PasteSymLinksClient client;
		client.NotifyServerOfResult(pastedItems);
	};

	auto clientLauncher = [&thread, clientThreadBody]
	{
		thread = std::jthread(clientThreadBody);
		return true;
	};

	auto receivedItems = server.LaunchClientAndWaitForResponse(clientLauncher, 1s);
	EXPECT_EQ(receivedItems, pastedItems);
}

TEST(PasteSymLinksServerClientTest, DataGreaterThanMaxSize)
{
	PasteSymLinksServer server;

	std::jthread thread;

	auto clientThreadBody = []
	{
		// It's not realistic for an individual file path to be this long in practice. But if there
		// are a large number of files on the clipboard and symlinks to those files are pasted, the
		// memory limit could be reached. So, this is a simple way of simulating a situation where
		// the serialized data is larger than the shared memory segment.
		// Note that SHARED_MEMORY_SIZE is in bytes and the path here is a std::wstring, so the
		// string will take twice that number of bytes. It's therefore guaranteed to not fit in the
		// shared memory segment.
		auto filePath =
			L"C:\\" + std::wstring(PasteSymLinksServerClientBase::SHARED_MEMORY_SIZE, '0');
		ClipboardOperations::PastedItems pastedItems = { { filePath, {} } };

		PasteSymLinksClient client;
		client.NotifyServerOfResult(pastedItems);
	};

	auto clientLauncher = [&thread, clientThreadBody]
	{
		thread = std::jthread(clientThreadBody);
		return true;
	};

	// The data the client is attempting to send is larger than the shared memory size. In that
	// case, no data should be received.
	auto receivedItems = server.LaunchClientAndWaitForResponse(clientLauncher, 1s);
	EXPECT_TRUE(receivedItems.empty());
}

TEST(PasteSymLinksServerClientTest, ClientLaunchFails)
{
	// If the client fails to launch, an empty result should be returned.
	PasteSymLinksServer server;
	auto receivedItems = server.LaunchClientAndWaitForResponse([] { return false; }, 1s);
	EXPECT_TRUE(receivedItems.empty());
}

TEST(PasteSymLinksServerClientTest, ClientFailsToSendResults)
{
	// If the client doesn't send back any results, an empty result should be returned after the
	// timeout.
	PasteSymLinksServer server;
	auto receivedItems = server.LaunchClientAndWaitForResponse([] { return true; }, 1s);
	EXPECT_TRUE(receivedItems.empty());
}

TEST(PasteSymLinksServerClientTest, NoServer)
{
	ClipboardOperations::PastedItems pastedItems = {
		{ L"C:\\file1", {} },
		{ L"C:\\file2", {} },
		{ L"C:\\file3", {} },
	};

	// If the server isn't present, the shared memory segment won't have been set up. This call
	// should be safe, but do nothing.
	PasteSymLinksClient client;
	client.NotifyServerOfResult(pastedItems);
}
