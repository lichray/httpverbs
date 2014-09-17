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

#ifndef HTTPVERBS_REQUEST_H
#define HTTPVERBS_REQUEST_H

#include "response.h"

#include <string>
#include <memory>
#include <cstdint>
#include <functional>
#include <stdexcept>

namespace httpverbs
{

struct _mini_string_ref;

namespace keywords
{

_mini_string_ref data_from(char const*);
_mini_string_ref data_from(char const*, size_t);

template <typename StringLike>
_mini_string_ref data_from(StringLike const&);

}

struct _mini_string_ref
{
	char const* data() const
	{
		return it_;
	}

	size_t size() const
	{
		return sz_;
	}

	size_t copy(char* s, size_t n) const;
	void remove_prefix(size_t n);

private:
	friend _mini_string_ref keywords::data_from(char const*);
	friend _mini_string_ref keywords::data_from(char const*, size_t);

	template <typename StringLike>
	friend _mini_string_ref keywords::data_from(StringLike const&);

	_mini_string_ref(char const* str, size_t len) :
		it_(str), sz_(len)
	{}

	char const* it_;
	size_t sz_;
};

struct request
{
private:
	struct _curl_handle_deleter
	{
		void operator()(void*) const;
	};

	std::unique_ptr<void, _curl_handle_deleter> handle_;

public:
	typedef std::function<size_t(char*, size_t)>	callback_t;
	typedef long long				length_t;

	std::string url;
	header_dict headers;
	std::string content;

	request(char const* method, std::string url);

	request(request&& other);
	request& operator=(request other);

	friend
	void swap(request& a, request& b);

	friend
	bool operator==(request const& a, request const& b)
	{
		return a.handle_ == b.handle_;
	}

	friend
	bool operator!=(request const& a, request const& b)
	{
		return !(a == b);
	}

	request& allow_redirects();
	request& ignore_response_body();

	response perform();
	response perform(callback_t writer);
	response perform(_mini_string_ref);
	response perform(_mini_string_ref, callback_t writer);
	response perform(length_t n, callback_t reader);
	response perform(length_t n, callback_t reader, callback_t writer);

private:
	request(request const&);  // = delete

	void setup_request_body_from_bytes(void* p, length_t n);
	void setup_request_body_from_callback(void* p, length_t n);
	void setup_response_body_to_string(void* p);
	void setup_response_body_to_callback(void* p);
	void perform_on(response& resp);
};

inline
request::request(request&& other) :
	handle_(std::move(other.handle_)),
	url(std::move(other.url)),
	headers(std::move(other.headers)),
	content(std::move(other.content))
{}

inline
request& request::operator=(request other)
{
	swap(*this, other);

	return *this;
}

inline
void swap(request& a, request& b)
{
	using std::swap;

	swap(a.handle_, b.handle_);
	swap(a.url, b.url);
	swap(a.headers, b.headers);
	swap(a.content, b.content);
}

inline
response request::perform()
{
	return perform(keywords::data_from(content));
}

inline
response request::perform(callback_t writer)
{
	return perform(keywords::data_from(content), std::move(writer));
}

inline
response request::perform(_mini_string_ref sv)
{
	setup_request_body_from_bytes(&sv, sv.size());

	response resp;
	setup_response_body_to_string(&resp.content);

	perform_on(resp);

	return resp;
}

inline
response request::perform(_mini_string_ref sv, callback_t writer)
{
	setup_request_body_from_bytes(&sv, sv.size());

	response resp;
	setup_response_body_to_callback(&writer);

	perform_on(resp);

	return resp;
}

inline
response request::perform(length_t n, callback_t reader)
{
	response resp;
	setup_request_body_from_callback(&reader, n);
	setup_response_body_to_string(&resp.content);

	perform_on(resp);

	return resp;
}

inline
response request::perform(length_t n, callback_t reader, callback_t writer)
{
	response resp;
	setup_request_body_from_callback(&reader, n);
	setup_response_body_to_callback(&writer);

	perform_on(resp);

	return resp;
}

namespace keywords
{

inline
_mini_string_ref data_from(char const* str)
{
	return data_from(str, std::char_traits<char>::length(str));
}

inline
_mini_string_ref data_from(char const* str, size_t len)
{
	return _mini_string_ref(str, len);
}

template <typename StringLike>
inline
_mini_string_ref data_from(StringLike const& str)
{
	return data_from(str.data(), str.size());
}

}

inline
size_t _mini_string_ref::copy(char* s, size_t n) const
{
	auto rlen = n < size() ? n : size();

	std::char_traits<char>::copy(s, data(), rlen);

	return rlen;
}

inline
void _mini_string_ref::remove_prefix(size_t n)
{
	it_ += n;
	sz_ -= n;
}

}

#endif
