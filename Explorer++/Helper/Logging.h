#pragma once

#include <boost\log\common.hpp>
#include <boost\log\sources\global_logger_storage.hpp>
#include <boost\log\sources\severity_logger.hpp>

enum SeverityLevel
{
	debug,
	info,
	warning,
	error,
	fatal
};

template<typename CharT, typename TraitsT>
inline std::basic_ostream<CharT, TraitsT>& operator<< (
	std::basic_ostream<CharT, TraitsT>& stream, SeverityLevel level)
{
	static const char* const str[] =
	{
		"debug",
		"info",
		"warning",
		"error",
		"fatal"
	};

	if (static_cast<std::size_t>(level) < (sizeof(str) / sizeof(*str)))
	{
		stream << str[level];
	}
	else
	{
		stream << static_cast<int>(level);
	}

	return stream;
}

BOOST_LOG_GLOBAL_LOGGER(logger, boost::log::sources::wseverity_logger_mt<SeverityLevel>)

#define LOG(severity) BOOST_LOG_SEV(logger::get(), severity)