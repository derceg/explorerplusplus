// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

// Represents a unique ID for a thread. Unlike std::thread::id, values are never reused.
class UniqueThreadId
{
public:
	static UniqueThreadId GetForCurrentThread();

	bool operator==(const UniqueThreadId &) const = default;

private:
	explicit UniqueThreadId(int threadId);

	const int m_threadId;
};
