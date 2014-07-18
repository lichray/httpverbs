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

#include "config.h"

#include <algorithm>
#include <iterator>
#include <utility>
#include <stdexcept>
#include <string.h>

#if defined(HAVE_STRINGS_H)
#include <strings.h>
#else
# if defined(HAVE__STRICMP)
# define strcasecmp	_stricmp
# define strncasecmp	_strnicmp
# endif
#endif

namespace httpverbs
{

namespace
{

struct header_compare
{
	explicit header_compare(size_t name_len) :
		name_len(name_len)
	{}

	bool operator()(std::string const& a, char const* b)
	{
		return b_cmp(a.data(), a.find(':'), b, name_len);
	}

	bool operator()(char const* a, std::string const& b)
	{
		return b_cmp(a, name_len, b.data(), b.find(':'));
	}

	bool operator()(std::string const& a, std::string const& b)
	{
		return b_cmp(a.data(), a.find(':'), b.data(), b.find(':'));
	}

private:
	static
	bool b_cmp(char const* a, size_t alen, char const* b, size_t blen)
	{
		auto rlen = std::min(alen, blen);
		auto r = strncasecmp(a, b, rlen);

		if (r == 0)
			return alen < blen;
		else
			return r < 0;
	}

	size_t name_len;
};

}

using std::begin;
using std::end;

template <typename Container>
inline
auto next_position(Container& c, char const* name, size_t name_len)
	-> decltype(begin(c))
{
	return std::upper_bound(begin(c), end(c), name,
	    header_compare(name_len));
}

template <typename Container>
inline
auto header_range(Container& c, char const* name, size_t name_len)
	-> decltype(std::make_pair(begin(c), end(c)))
{
	return std::equal_range(begin(c), end(c), name,
	    header_compare(name_len));
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

template <typename Iter>
inline
std::string join_trimmed_values(Iter first, Iter last, size_t name_len)
{
	std::string v;
	auto it = first;

	while (1)
	{
		auto fc = trimmed_range(begin(*it) + name_len + 1, end(*it));
		v.append(fc.first, fc.second);

		if (++it != last)
			v.append(", ");
		else
			break;
	}

	return v;
}

inline
std::string b_joined(char const* a, size_t alen, char const* b, size_t blen)
{
	std::string header;

	header.reserve(alen + 2 + blen);
	header.append(a, alen).append(": ").append(b, blen);

	return header;
}

void header_dict::add(char const* name, char const* value)
{
	auto nl = strlen(name);
	auto vl = strlen(value);

	add_line_split_at(b_joined(name, nl, value, vl), nl);
}

void header_dict::add(std::string header)
{
	auto pos = header.find(':');

	if (pos == std::string::npos)
		return;

	add_line_split_at(std::move(header), pos);
}

void header_dict::add_line_split_at(std::string header, size_t pos)
{
	auto it = next_position(hlist_, header.data(), pos);

	hlist_.insert(it, std::move(header));
}

auto header_dict::operator[](char const* name) const -> value_type
{
	auto name_len = strlen(name);
	auto hr = header_range(hlist_, name, name_len);

	if (hr.first == hr.second)
		throw std::out_of_range("no such header");

	return join_trimmed_values(hr.first, hr.second, name_len);
}

auto header_dict::get(char const* name) const -> boost::optional<value_type>
{
	auto name_len = strlen(name);
	auto hr = header_range(hlist_, name, name_len);

	if (hr.first == hr.second)
		return boost::none;

	return join_trimmed_values(hr.first, hr.second, name_len);
}

void header_dict::set(char const* name, char const* value)
{
	auto nl = strlen(name);
	auto vl = strlen(value);
	auto hr = header_range(hlist_, name, nl);

	if (hr.first == hr.second)
		hlist_.insert(hr.first, b_joined(name, nl, value, vl));
	else
	{
		hr.first->replace(nl + 1, -1, 1, ' ').append(value, vl);
		++hr.first;
		hlist_.erase(hr.first, hr.second);
	}
}

auto header_dict::erase(char const* name) -> size_type
{
	auto hr = header_range(hlist_, name, strlen(name));
	auto diff = hr.second - hr.first;

	hlist_.erase(hr.first, hr.second);

	return diff;
}

}
