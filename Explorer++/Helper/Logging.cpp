#include "stdafx.h"
#include "Logging.h"

BOOST_LOG_GLOBAL_LOGGER_DEFAULT(logger, boost::log::sources::wseverity_logger_mt<SeverityLevel>)