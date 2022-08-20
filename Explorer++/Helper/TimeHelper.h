// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

BOOL LocalSystemTimeToFileTime(const SYSTEMTIME *lpLocalTime, FILETIME *lpFileTime);
BOOL FileTimeToLocalSystemTime(const FILETIME *lpFileTime, SYSTEMTIME *lpLocalTime);
void MergeDateTime(SYSTEMTIME *pstOutput, const SYSTEMTIME *pstDate, const SYSTEMTIME *pstTime);
