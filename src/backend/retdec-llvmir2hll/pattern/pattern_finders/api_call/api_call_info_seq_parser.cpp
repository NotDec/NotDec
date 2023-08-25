/**
* @file src/llvmir2hll/pattern/pattern_finders/api_call/api_call_info_seq_parser.cpp
* @brief Implementation of APICallInfoSeqParser.
* @copyright (c) 2017 Avast Software, licensed under the MIT license
*/

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <functional>

#include "retdec/llvmir2hll/pattern/pattern_finders/api_call/api_call_info_seq_parser.h"
#include "retdec/llvmir2hll/support/debug.h"

namespace retdec {
namespace llvmir2hll {

namespace {

/**
* @brief Returns @c true if the given character can be a part of an ID, @c
*        false otherwise.
*/
bool isIdChar(std::string::value_type c) {
	return std::isalnum(c) || c == '_';
}

/**
* @brief Returns @c true of the given token represents an ID, @c false
*        otherwise.
*
* A token represents an ID if it contains only alphanumeric characters and
* underscores ('_').
*/
bool isId(const std::string &token) {
	return std::find_if(
			token.begin(),
			token.end(),
			[](char c){return !isIdChar(c);}) == token.end();
}

/**
* @brief Tokenizes the given string.
*
* If there is a lexical error, the empty vector is returned.
*/
StringVector tokenize(const std::string &str) {
	StringVector tokens;
	std::size_t i = 0;
	while (i < str.size()) {
		if (std::isspace(str[i])) {
			++i;
			continue;
		}

		std::string token;
		if (str[i] == '(' || str[i] == ')' || str[i] == '=' ||
				str[i] == ',' || str[i] == ';') {
			token = str[i];
			++i;
		} else if (isIdChar(str[i])) {
			do {
				token += str[i];
				++i;
			} while (isIdChar(str[i]));
		} else {
			// Unexpected character.
			return StringVector();
		}
		tokens.push_back(token);
	}
	return tokens;
}

/**
* @brief Returns the number of tokens that are left in @a tokens, assuming that
*        the current token is on index @a i.
*/
std::size_t left(const StringVector &tokens, std::size_t i) {
	return tokens.size() - i;
}

/**
* @brief Parses APICallInfo from @a tokens, starting at @a i, and advances @a i.
*
* @return @c info if the info was parsed correctly,
*         @c std::nullopt otherwise.
*/
std::optional<APICallInfo> parseAPICallInfo(const StringVector &tokens, std::size_t &i) {
	if (left(tokens, i) < 3) {
		return std::nullopt;
	}

	// consume `X =` (optional)
	std::string returnValueBind;
	if (isId(tokens[i]) && tokens[i+1] == "=") {
		returnValueBind = tokens[i];
		i += 2;
	}

	// consume `func(`
	if (left(tokens, i) < 2 || !isId(tokens[i]) ||
			tokens[i+1] != "(") {
		return std::nullopt;
	}
	APICallInfo info(tokens[i]);
	i += 2;

	if (!returnValueBind.empty()) {
		info.bindReturnValue(returnValueBind);
	}

	// consume `args` (optional)
	unsigned n = 1;
	while (left(tokens, i) >= 2 && tokens[i] != ")") {
		// expect `X` or `_`
		if (!isId(tokens[i]) && tokens[i] != "_") {
			return std::nullopt;
		}

		// consume `X` or `_`
		if (isId(tokens[i]) && tokens[i] != "_") {
			info.bindParam(n, tokens[i]);
		}
		++n;
		++i;

		// expect `,` or `)` and consume `,`
		if (tokens[i] == ",") {
			++i;
		} else if (tokens[i] != ")") {
			return std::nullopt;
		}
	}

	// consume `)`
	if (left(tokens, i) < 1 || tokens[i] != ")") {
		return std::nullopt;
	}
	++i;

	// consume `;` (optional)
	if (left(tokens, i) > 0 && tokens[i] == ";") {
		++i;
	}

	return info;
}

} // anonymous namespace

/**
* @brief Constructs an APICallInfoSeqParser instance.
*/
APICallInfoSeqParser::APICallInfoSeqParser() {}

/**
* @brief Parses the given text into a sequence of API calls information.
*
* See the class description for a description of the format of @a text.
*
* If @a text is invalid, it returns @c std::nullopt. If @a
* text is empty, it returns an empty sequence.
*/
std::optional<APICallInfoSeq> APICallInfoSeqParser::parse(const std::string &text) const {
	if (text.empty()) {
		return APICallInfoSeq();
	}

	StringVector tokens(tokenize(text));
	if (tokens.empty()) {
		return std::nullopt;
	}

	APICallInfoSeq seq;
	std::size_t i = 0;
	while (left(tokens, i) > 0) {
		std::optional<APICallInfo> info(parseAPICallInfo(tokens, i));
		if (!info) {
			return std::nullopt;
		}
		seq.add(info.value());
	}
	return seq;
}

/**
* @brief Creates a new parser.
*/
ShPtr<APICallInfoSeqParser> APICallInfoSeqParser::create() {
	return ShPtr<APICallInfoSeqParser>(new APICallInfoSeqParser());
}

} // namespace llvmir2hll
} // namespace retdec
