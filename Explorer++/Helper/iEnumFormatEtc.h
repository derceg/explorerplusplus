#ifndef ENUMFORMATETC_INCLUDED
#define ENUMFORMATETC_INCLUDED

#include <list>

#pragma once

HRESULT CreateEnumFormatEtc(std::list<FORMATETC> feList,IEnumFORMATETC **ppEnumFormatEtc);

#endif