/******************************************************************
 *
 * Project: Explorer++
 * File: Logging.cpp
 * License: GPL - See LICENSE in the top level directory
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Logging.h"
#include "../Helper/Logging.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"
#include <boost/locale/generator.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", SeverityLevel);

void InitializeLogging(const TCHAR *filename)
{
	TCHAR szLogFile[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), szLogFile, SIZEOF_ARRAY(szLogFile));

	PathRemoveFileSpec(szLogFile);
	PathAppend(szLogFile, filename);

	boost::log::add_common_attributes();

	boost::shared_ptr<boost::log::sinks::synchronous_sink<boost::log::sinks::text_file_backend>> sink = boost::log::add_file_log(
		boost::log::keywords::file_name = szLogFile,
		boost::log::keywords::open_mode = std::ios_base::app,
		boost::log::keywords::format =
		(
			boost::log::expressions::stream
			<< "[" << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")
			<< "; " << severity.or_default(info)
			<< "]: " << boost::log::expressions::message
			)
	);

	std::locale locale = boost::locale::generator()("en_US.UTF-8");
	sink->imbue(locale);
}