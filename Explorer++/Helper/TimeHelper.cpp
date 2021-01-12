// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TimeHelper.h"

BOOL LocalSystemTimeToFileTime(const SYSTEMTIME *lpLocalTime, FILETIME *lpFileTime)
{
	SYSTEMTIME systemTime;
	BOOL result = TzSpecificLocalTimeToSystemTime(nullptr, lpLocalTime, &systemTime);

	if (result)
	{
		result = SystemTimeToFileTime(&systemTime, lpFileTime);
	}

	return result;
}

BOOL FileTimeToLocalSystemTime(const FILETIME *lpFileTime, SYSTEMTIME *lpLocalTime)
{
	SYSTEMTIME systemTime;
	BOOL result = FileTimeToSystemTime(lpFileTime, &systemTime);

	if (result)
	{
		result = SystemTimeToTzSpecificLocalTime(nullptr, &systemTime, lpLocalTime);
	}

	return result;
}

void MergeDateTime(SYSTEMTIME *pstOutput, const SYSTEMTIME *pstDate, const SYSTEMTIME *pstTime)
{
	/* Date fields. */
	pstOutput->wYear = pstDate->wYear;
	pstOutput->wMonth = pstDate->wMonth;
	pstOutput->wDayOfWeek = pstDate->wDayOfWeek;
	pstOutput->wDay = pstDate->wDay;

	/* Time fields. */
	pstOutput->wHour = pstTime->wHour;
	pstOutput->wMinute = pstTime->wMinute;
	pstOutput->wSecond = pstTime->wSecond;
	pstOutput->wMilliseconds = pstTime->wMilliseconds;
}