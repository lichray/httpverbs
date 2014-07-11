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

#include <httpverbs/header_dict.h>

#include <algorithm>
#include <iterator>
#include <utility>
#include <stdexcept>
#include "strings.h"

namespace httpverbs
{

template <typename Container>
inline
auto header_position(Container& c, char const* name, size_t name_len)
	-> decltype(std::begin(c))  // remove this line in c++14
{
	using std::begin;
	using std::end;

	return std::lower_bound(begin(c), end(c), name,
	    [=](std::string const& a, char const* b) -> bool
	    {
		auto elen = a.find(':');
		auto rlen = std::min(elen, name_len);
		auto r = strncasecmp(a.data(), b, rlen);

		if (r == 0)
			return elen < name_len;
		else
			return r < 0;
	    });
}

template <typename Iter>
inline
auto make_reverse_iterator(Iter it)
	-> std::reverse_iterator<Iter>
{
	return std::reverse_iterator<Iter>(it);
}

template <typename BidirIt>
inline
auto trimmed_range(BidirIt first, BidirIt last)
	-> std::pair<BidirIt, BidirIt>
{
	// it's libcurl's job to concatenate multi-line headers,
	// so HTTP LWS actually means SP and HT here
	auto is_LWS = [](char c)
	{
		return c == ' ' or c == '\t';
	};

	// trim
	auto fc_b = std::find_if_not(first, last, is_LWS);
	auto fc_e = std::find_if_not(make_reverse_iterator(last),
	    make_reverse_iterator(fc_b), is_LWS).base();

	return std::make_pair(fc_b, fc_e);
}

void header_dict::add(char const* name, char const* value)
{
	auto nv = strlen(name);
	auto lv = strlen(value);

	std::string header;
	header.reserve(nv + 2 + lv);

	header.append(name, nv).append(": ").append(value, lv);
	add_line_split_at(std::move(header), nv);
}

void header_dict::add_line(std::string header)
{
	auto pos = header.find(':');

	if (pos == std::string::npos)
		return;

	add_line_split_at(std::move(header), pos);
}

void header_dict::add_line_split_at(std::string header, size_t pos)
{
	auto it = header_position(hlist_, header.data(), pos);

	hlist_.insert(it, std::move(header));
}

auto header_dict::operator[](char const* name) const -> value_type
{
	// XXX not yet support duplicated field-names
	auto name_len = strlen(name);
	auto it = header_position(hlist_, name, name_len);

	if (it == end(hlist_))
		throw std::out_of_range("no such header");

	auto& hl = *it;

	if (strncasecmp(hl.data(), name, name_len) != 0)
		throw std::out_of_range("no such header");

	auto fc = trimmed_range(begin(hl) + name_len + 1, end(hl));

	return std::string(fc.first, fc.second);
}

auto header_dict::get(char const* name) const -> boost::optional<value_type>
{
	typedef boost::optional<std::string> R;

	// XXX not yet support duplicated field-names
	auto name_len = strlen(name);
	auto it = header_position(hlist_, name, name_len);

	if (it == end(hlist_))
		return boost::none;

	auto& hl = *it;

	if (strncasecmp(hl.data(), name, name_len) != 0)
		return boost::none;

	auto fc = trimmed_range(begin(hl) + name_len + 1, end(hl));

	return R(boost::in_place(fc.first, fc.second));
}

}
