// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CommandLineSplitter.h"
#include <tao/pegtl.hpp>

namespace pegtl = TAO_PEGTL_NAMESPACE;

namespace CommandLineSplitterGrammar
{

struct Whitespace : pegtl::one<' ', '\t'>
{
};

struct Quote : pegtl::one<'"'>
{
};

template <typename... Rules>
struct ArgumentExcludingCharacterTypes :
	pegtl::plus<pegtl::not_at<pegtl::sor<Rules...>>, pegtl::any>
{
};

struct PlainArgument : ArgumentExcludingCharacterTypes<Whitespace, Quote>
{
};

struct QuotedArgumentContent : ArgumentExcludingCharacterTypes<Quote>
{
};

struct QuotedArgumentTail : pegtl::seq<pegtl::opt<QuotedArgumentContent>, Quote>
{
};

struct QuotedArgument : pegtl::if_must<Quote, QuotedArgumentTail>
{
};

struct Argument : pegtl::sor<QuotedArgument, PlainArgument>
{
};

// pegtl::list<> will fail to match if there isn't at least one argument, which is the desired
// behavior, since there should always be at least one argument on the command line (the executable
// name).
struct CommandLineContent : pegtl::list<Argument, pegtl::plus<Whitespace>>
{
};

struct PaddedCommandLineContent : pegtl::seq<pegtl::pad<CommandLineContent, Whitespace>, pegtl::eof>
{
};

struct CommandLine : pegtl::must<PaddedCommandLineContent>
{
};

template <typename Rule>
struct Action
{
};

template <>
struct Action<PlainArgument>
{
	template <typename ParseInput>
	static void apply(const ParseInput &input, std::vector<std::string> &output)
	{
		output.push_back(input.string());
	}
};

template <>
struct Action<QuotedArgumentContent> : Action<PlainArgument>
{
};

template <typename>
inline constexpr const char *error_message = nullptr;

template <>
inline constexpr const char *error_message<QuotedArgumentTail> =
	"Closing quote missing in argument list";

// Essentially, this rule fails to match if the argument list can't be matched. That can happen if
// the argument list is empty (the pegtl::list<> rule above expects a non-empty list) or if there is
// data outside of a quoted argument. For example, parsing will fail when the argument list
// contains an argument like one of the following:
//
// "C:\"E:\path\to\directory
// C:\"E:\path\to\directory"
template <>
inline constexpr const char *error_message<PaddedCommandLineContent> =
	"Couldn't parse command line. Check there is at least one argument and arguments are quoted correctly.";

struct Error
{
	template <typename Rule>
	static constexpr bool raise_on_failure = false;
	template <typename Rule>
	static constexpr auto message = error_message<Rule>;
};

template <typename Rule>
using Control = pegtl::must_if<Error>::control<Rule>;

}

namespace CommandLineSplitter
{

Result Split(const std::string &commandLine)
{
	pegtl::memory_input input(commandLine, "Command line");
	std::vector<std::string> output;

	try
	{
		// pegtl::parse() can also return false in case of a local failure. However, that's not
		// possible in this case, since a pegtl::must<> rule is applied to the top-level rule. That
		// means that if parsing fails, an exception will always be thrown.
		pegtl::parse<CommandLineSplitterGrammar::CommandLine, CommandLineSplitterGrammar::Action,
			CommandLineSplitterGrammar::Control>(input, output);
	}
	catch (const pegtl::parse_error &e)
	{
		return { e.what() };
	}

	return output;
}

}
