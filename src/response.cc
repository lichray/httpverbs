/*-
 * Copyright (c) 2014 Zhihao Yuan.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <httpverbs/response.h>

#include <algorithm>
#include "strings.h"

namespace httpverbs
{

namespace
{

struct header_comparator
{
	bool operator()(std::string const& header_line, char const* field_name)
	{
		auto pos = header_line.find(':');

		// non-headers, if exist, are just collected at the
		// beginning of the list
		if (pos == std::string::npos)
			return true;

		return (strncasecmp(header_line.data(), field_name, pos) < 0);
	}
};

}

boost::optional<std::string> response::get_header(char const* name) const
{
	typedef boost::optional<std::string>		R;
	typedef std::string::const_reverse_iterator	reversed;

	// XXX not yet support duplicated field-names
	auto it = std::lower_bound(begin(headers_), end(headers_), name,
	    header_comparator());

	if (it == end(headers_))
		return boost::none;

	auto& hl = *it;
	auto pos = hl.find(':');

	if (strncasecmp(hl.data(), name, pos) != 0)
		return boost::none;

	// it's libcurl's job to concatenate multi-line headers,
	// so HTTP LWS actually means SP and HT here
	auto is_LWS = [](char c)
	{
		return c == ' ' or c == '\t';
	};

	auto hl_b = begin(hl);
	auto hl_e = end(hl);

	// trim
	auto fc_b = std::find_if_not(hl_b + pos + 1, hl_e, is_LWS);
	auto fc_e = std::find_if_not(reversed(hl_e), reversed(fc_b),
	    is_LWS).base();

	return R(boost::in_place(fc_b, fc_e));
}

}
