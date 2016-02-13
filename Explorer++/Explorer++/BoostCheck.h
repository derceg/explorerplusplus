#pragma once

#define REQUIRED_BOOST_VERSION 106000

#include <boost\version.hpp>
#include <boost\preprocessor\stringize.hpp>

static_assert(BOOST_VERSION == REQUIRED_BOOST_VERSION,
	"Version " BOOST_PP_STRINGIZE(REQUIRED_BOOST_VERSION)
	" of Boost is required; found version "
	BOOST_PP_STRINGIZE(BOOST_VERSION));