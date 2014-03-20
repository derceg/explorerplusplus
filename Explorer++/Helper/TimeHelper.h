#pragma once

BOOL LocalSystemTimeToFileTime(const LPSYSTEMTIME lpLocalTime, LPFILETIME lpFileTime);
BOOL FileTimeToLocalSystemTime(const LPFILETIME lpFileTime, LPSYSTEMTIME lpLocalTime);
void MergeDateTime(SYSTEMTIME *pstOutput, const SYSTEMTIME *pstDate, const SYSTEMTIME *pstTime);