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

#include "header_dict.h"

#include <type_traits>
#include <string>
#include <memory>
#include <cstdint>
#include <functional>

namespace httpverbs
{

const struct from_data_t {} from_data = {};
const struct to_content_t {} to_content = {};
const struct ignoring_response_body_t {} ignoring_response_body = {};

struct of_length
{
	explicit of_length(int n) : v_(n)
	{}

	explicit of_length(long n) : v_(n)
	{}

	explicit of_length(long long n);
	explicit of_length(size_t n);

	int64_t value() const;

private:
	int64_t v_;
};

struct response;

struct request
{
	typedef std::function<size_t(char*, size_t)> callback_t;

	std::string url;
	header_dict headers;
	std::string data;

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

	response perform();
	response perform(from_data_t, to_content_t);
	response perform(from_data_t, callback_t writer);
	response perform(from_data_t, ignoring_response_body_t);
	response perform(callback_t reader, of_length n, to_content_t);
	response perform(callback_t reader, of_length n, callback_t writer);
	response perform(callback_t reader, of_length n,
	    ignoring_response_body_t);

private:
	request(request const&);  // = delete

	void setup_request_body_from_bytes(void* p, of_length n);
	void setup_request_body_from_callback(void* p, of_length n);
	void setup_response_body_to_string(void* p);
	void setup_response_body_to_callback(void* p);
	void setup_response_body_ignored();
	void setup_response_headers(void* p);
	void perform_on(response& resp);

	struct _curl_handle_deleter
	{
		void operator()(void*) const;
	};

	std::unique_ptr<void, _curl_handle_deleter> handle_;
};

inline
of_length::of_length(long long n) : v_(n)
{
	static_assert(sizeof(n) <= sizeof(v_), "bug me if you see this");
}

inline
int64_t of_length::value() const
{
	return v_;
}

inline
request::request(request&& other) :
	url(std::move(other.url)),
	headers(std::move(other.headers)),
	data(std::move(other.data)),
	handle_(std::move(other.handle_))
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

	swap(a.url, b.url);
	swap(a.headers, b.headers);
	swap(a.data, b.data);
	swap(a.handle_, b.handle_);
}

}

#endif
