/******************************************************************
*
* Project: Helper
* File: TimeHelper.cpp
* License: GPL - See COPYING in the top level directory
*
* Time-related helper functionality.
*
* Written by David Erceg
* www.explorerplusplus.com
*
*****************************************************************/

#include "stdafx.h"
#include "TimeHelper.h"


BOOL LocalSystemTimeToFileTime(const LPSYSTEMTIME lpLocalTime, LPFILETIME lpFileTime)
{
	SYSTEMTIME SystemTime;
	BOOL result = TzSpecificLocalTimeToSystemTime(NULL, lpLocalTime, &SystemTime);

	if(result)
	{
		result = SystemTimeToFileTime(&SystemTime, lpFileTime);
	}

	return result;
}

BOOL FileTimeToLocalSystemTime(const LPFILETIME lpFileTime, LPSYSTEMTIME lpLocalTime)
{
	SYSTEMTIME SystemTime;
	BOOL result = FileTimeToSystemTime(lpFileTime, &SystemTime);

	if(result)
	{
		result = SystemTimeToTzSpecificLocalTime(NULL, &SystemTime, lpLocalTime);
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