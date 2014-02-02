#pragma once

#include <list>

HRESULT CreateEnumFormatEtc(const std::list<FORMATETC> &feList, IEnumFORMATETC **ppEnumFormatEtc);