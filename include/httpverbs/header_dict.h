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

#ifndef HTTPVERBS_HEADER__DICT_H
#define HTTPVERBS_HEADER__DICT_H

#include <boost/optional.hpp>
#include <boost/assert.hpp>

#include <string>
#include <vector>
#include <algorithm>
#include <iterator>

#if defined(_MSC_VER)
#define NOMINMAX
#include <ciso646>
#endif

namespace httpverbs
{

struct _mini_ntmbs
{
	_mini_ntmbs(char const* str) : it_(str)
	{}

	_mini_ntmbs(std::string const& str) : it_(str.data())
	{}

	operator char const*() const
	{
		return it_;
	}

private:
	char const* it_;
};

struct header_dict
{
private:
	typedef std::vector<std::string> _Rep;
	_Rep hlist_;

public:
	typedef _Rep::value_type	value_type;
	typedef _Rep::size_type		size_type;
	typedef _Rep::iterator		iterator;
	typedef _Rep::const_iterator	const_iterator;

	header_dict() {}

#if defined(_MSC_VER) && _MSC_VER < 1900
	header_dict(header_dict&& other);
	header_dict& operator=(header_dict&& other);
#endif

	friend
	bool operator==(header_dict const& a, header_dict const& b)
	{
		return a.hlist_ == b.hlist_;
	}

	friend
	bool operator!=(header_dict const& a, header_dict const& b)
	{
		return !(a == b);
	}

	value_type operator[](_mini_ntmbs name) const;
	boost::optional<value_type> get(_mini_ntmbs name) const;

	void clear();
	void add(std::string header);
	void add(_mini_ntmbs name, _mini_ntmbs value);
	void set(_mini_ntmbs name, _mini_ntmbs value);
	size_type erase(_mini_ntmbs name);

	friend const_iterator begin(header_dict const& d);
	friend const_iterator end(header_dict const& d);

	bool empty() const;
	size_type size() const;

private:
	void add_line_split_at(std::string&& header, size_t pos);

	auto next_position(char const* name, size_t name_len)
		-> iterator;
	auto matched_range(char const* name, size_t name_len)
		-> std::pair<iterator, iterator>;
	auto matched_range(char const* name, size_t name_len) const
		-> std::pair<const_iterator, const_iterator>;

	static std::string fields_joined(char const*, size_t, char const*,
	    size_t);
	static value_type joined_field_values(const_iterator, const_iterator,
	    size_t);
};

#if defined(_MSC_VER) && _MSC_VER < 1900

inline
header_dict::header_dict(header_dict&& other) :
	hlist_(std::move(other.hlist_))
{}

inline
header_dict& header_dict::operator=(header_dict&& other)
{
	hlist_ = std::move(other.hlist_);

	return *this;
}

#endif

inline
void header_dict::clear()
{
	hlist_.clear();
}

inline
header_dict::const_iterator begin(header_dict const& d)
{
	return d.hlist_.begin();
}

inline
header_dict::const_iterator end(header_dict const& d)
{
	return d.hlist_.end();
}

inline
bool header_dict::empty() const
{
	return hlist_.empty();
}

inline
auto header_dict::size() const -> size_type
{
	return hlist_.size();
}

inline
void header_dict::add(_mini_ntmbs name, _mini_ntmbs value)
{
	auto nl = std::char_traits<char>::length(name);
	auto vl = std::char_traits<char>::length(value);

	add_line_split_at(fields_joined(name, nl, value, vl), nl);
}

inline
void header_dict::add(std::string header)
{
	auto pos = header.find(':');

	if (pos == std::string::npos)
		return;

	add_line_split_at(std::move(header), pos);
}

inline
auto header_dict::operator[](_mini_ntmbs name) const -> value_type
{
	auto name_len = std::char_traits<char>::length(name);
	auto hr = matched_range(name, name_len);

	BOOST_ASSERT_MSG(hr.first != hr.second, "no such header");

	return joined_field_values(hr.first, hr.second, name_len);
}

inline
auto header_dict::get(_mini_ntmbs name) const -> boost::optional<value_type>
{
	auto name_len = std::char_traits<char>::length(name);
	auto hr = matched_range(name, name_len);

	if (hr.first == hr.second)
		return boost::none;

	return joined_field_values(hr.first, hr.second, name_len);
}

inline
void header_dict::set(_mini_ntmbs name, _mini_ntmbs value)
{
	auto nl = std::char_traits<char>::length(name);
	auto vl = std::char_traits<char>::length(value);
	auto hr = matched_range(name, nl);

	if (hr.first == hr.second)
		hlist_.insert(hr.first, fields_joined(name, nl, value, vl));
	else
	{
		hr.first->replace(nl + 1, -1, 1, ' ').append(value, vl);
		++hr.first;
		hlist_.erase(hr.first, hr.second);
	}
}

inline
auto header_dict::erase(_mini_ntmbs name) -> size_type
{
	auto hr = matched_range(name, std::char_traits<char>::length(name));
	auto diff = hr.second - hr.first;

	hlist_.erase(hr.first, hr.second);

	return diff;
}

inline
void header_dict::add_line_split_at(std::string&& header, size_t pos)
{
	auto it = next_position(header.data(), pos);

	hlist_.insert(it, std::move(header));
}

inline
std::string header_dict::fields_joined(char const* a, size_t alen,
    char const* b, size_t blen)
{
	std::string header;

	header.reserve(alen + 2 + blen);
	header.append(a, alen).append(": ").append(b, blen);

	return header;
}

namespace
{

struct _header_compare
{
	explicit _header_compare(size_t name_len) :
		name_len_(name_len)
	{}

	bool operator()(std::string const& a, char const* b)
	{
		return b_cmp(a.data(), a.find(':'), b, name_len_);
	}

	bool operator()(char const* a, std::string const& b)
	{
		return b_cmp(a, name_len_, b.data(), b.find(':'));
	}

	bool operator()(std::string const& a, std::string const& b)
	{
		return b_cmp(a.data(), a.find(':'), b.data(), b.find(':'));
	}

private:
	static
	bool b_cmp(char const* a, size_t alen, char const* b,
	    size_t blen)
	{
		return std::lexicographical_compare(a, a + alen, b, b + blen,
		    [](char a, char b)
		    {
			return tolower_li(a) < tolower_li(b);
		    });
	}

	static
	int tolower_li(int c)
	{
		return ('A' <= c && c <= 'Z') ? (c | ('a' - 'A')) : c;
	}

	size_t name_len_;
};

}

inline
auto header_dict::next_position(char const* name, size_t name_len)
	-> iterator
{
	return std::upper_bound(begin(hlist_), end(hlist_), name,
	    _header_compare(name_len));
}

inline
auto header_dict::matched_range(char const* name, size_t name_len)
	-> std::pair<iterator, iterator>
{
	return std::equal_range(begin(hlist_), end(hlist_), name,
	    _header_compare(name_len));
}

inline
auto header_dict::matched_range(char const* name, size_t name_len) const
	-> std::pair<const_iterator, const_iterator>
{
	return std::equal_range(begin(hlist_), end(hlist_), name,
	    _header_compare(name_len));
}

template <typename Iter>
inline
auto _make_reverse_iterator(Iter it)
	-> std::reverse_iterator<Iter>
{
	return std::reverse_iterator<Iter>(it);
}

template <typename BidirIt>
inline
auto _trimmed_range(BidirIt first, BidirIt last)
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
	auto fc_e = std::find_if_not(_make_reverse_iterator(last),
	    _make_reverse_iterator(fc_b), is_LWS).base();

	return std::pair<BidirIt, BidirIt>(fc_b, fc_e);
}

inline
auto header_dict::joined_field_values(const_iterator first,
    const_iterator last, size_t name_len)
	-> value_type
{
	std::string v;
	auto it = first;

	while (1)
	{
		auto fc = _trimmed_range(begin(*it) + name_len + 1, end(*it));
		v.append(fc.first, fc.second);

		if (++it != last)
			v.append(", ");
		else
			break;
	}

	return v;
}

}

#endif
