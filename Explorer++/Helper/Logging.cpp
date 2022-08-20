// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Logging.h"

BOOST_LOG_GLOBAL_LOGGER_DEFAULT(logger, boost::log::sources::wseverity_logger_mt<SeverityLevel>)
