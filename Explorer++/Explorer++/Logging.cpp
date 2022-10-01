// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Logging.h"
#include "../Helper/Logging.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"
#include <boost/locale/generator.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/exception_handler.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", SeverityLevel);

void InitializeLogging(const TCHAR *filename)
{
	// TODO: The file should be created in a directory that's intended to be writeable, rather than
	// the application directory.
	TCHAR szLogFile[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), szLogFile, SIZEOF_ARRAY(szLogFile));

	PathRemoveFileSpec(szLogFile);
	PathAppend(szLogFile, filename);

	boost::log::add_common_attributes();

	// clang-format off
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
	// clang-format on

	// Creating or writing to the log file may fail. In that case, the error should simply be
	// ignored, as there's not a lot that can be done.
	sink->set_exception_handler(boost::log::make_exception_suppressor());

	std::locale locale = boost::locale::generator()("en_US.UTF-8");
	sink->imbue(locale);
}
