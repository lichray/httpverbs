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

#include <string>
#include <vector>

#include "config.h"

namespace httpverbs
{

struct header_dict
{
private:
	typedef std::vector<std::string> _Rep;
	_Rep hlist_;

public:
	typedef _Rep::value_type	value_type;
	typedef _Rep::size_type		size_type;
	typedef _Rep::const_iterator	const_iterator;
	typedef const_iterator		iterator;

	header_dict() {}

#if defined(_MSC_VER) && _MSC_VER < 1800
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

	value_type operator[](char const* name) const;
	value_type operator[](std::string const& name) const;

	boost::optional<value_type> get(char const* name) const;
	boost::optional<value_type> get(std::string const& name) const;

	void add(char const* name, char const* value);
	void add(char const* name, std::string const& value);
	void add(std::string const& name, char const* value);
	void add(std::string const& name, std::string const& value);

	void add_line(std::string header);

	friend const_iterator begin(header_dict const& d);
	friend const_iterator end(header_dict const& d);

	bool empty() const;
	size_type size() const;

private:
	void add_line_split_at(std::string header, size_t pos);
};

#if defined(_MSC_VER) && _MSC_VER < 1800

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
auto header_dict::operator[](std::string const& name) const -> value_type
{
	return (*this)[name.data()];
}

inline
auto header_dict::get(std::string const& name) const
	-> boost::optional<value_type>
{
	return get(name.data());
}

inline
void header_dict::add(std::string const& name, std::string const& value)
{
	add(name.data(), value.data());
}

inline
void header_dict::add(std::string const& name, char const* value)
{
	add(name.data(), value);
}

inline
void header_dict::add(char const* name, std::string const& value)
{
	add(name, value.data());
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

}

#endif
