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

#include <httpverbs/request.h>
#include <httpverbs/exceptions.h>

#include <boost/assert.hpp>

#include "pooled_perform.h"

namespace httpverbs
{

void request::_curl_handle_deleter::operator()(void* p) const
{
	curl_easy_cleanup(p);
}

static size_t read_string(char*, size_t, size_t, void*);
static size_t write_string(char*, size_t, size_t, void*);
static size_t call_function(char*, size_t, size_t, void*);
static size_t fill_headers(char*, size_t, size_t, void*);

namespace
{

struct headers_parser_stack
{
	bool done_status_line;
	header_dict& ls;
};

}

request::request(char const* method, std::string url) :
	handle_(curl_easy_init()),
	url(std::move(url))
{
	if (handle_ == nullptr)
		throw bad_request();

	if (curl_easy_setopt(handle_.get(), CURLOPT_CUSTOMREQUEST, method))
		throw bad_request();
}

request& request::allow_redirects()
{
	curl_easy_setopt(handle_.get(), CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(handle_.get(), CURLOPT_MAXREDIRS, 5L);

	return *this;
}

request& request::ignore_response_body()
{
	// use a pointer pointing to an invalid location to represent
	// that the response body is unwanted
	curl_easy_setopt(handle_.get(), CURLOPT_PRIVATE, "");

	return *this;
}

inline
bool response_body_ignored(CURL* handle)
{
	char* no_body;
	curl_easy_getinfo(handle, CURLINFO_PRIVATE, &no_body);

	return no_body != nullptr;
}

void request::setup_request_body_from_bytes(void* p, length_t n)
{
	auto sz = curl_off_t(n);

	if (sz != 0)
	{
		curl_easy_setopt(handle_.get(), CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(handle_.get(), CURLOPT_INFILESIZE_LARGE, sz);
		curl_easy_setopt(handle_.get(), CURLOPT_READFUNCTION,
		    read_string);
		curl_easy_setopt(handle_.get(), CURLOPT_READDATA, p);
	}
}

void request::setup_response_body_to_string(void* p)
{
	if (not response_body_ignored(handle_.get()))
	{
		curl_easy_setopt(handle_.get(), CURLOPT_WRITEFUNCTION,
		    write_string);
		curl_easy_setopt(handle_.get(), CURLOPT_WRITEDATA, p);
	}
	else
		curl_easy_setopt(handle_.get(), CURLOPT_NOBODY, 1L);
}

void request::setup_request_body_from_callback(void* p, length_t n)
{
	auto sz = curl_off_t(n);

	if (sz != 0)
	{
		curl_easy_setopt(handle_.get(), CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(handle_.get(), CURLOPT_INFILESIZE_LARGE, sz);
		curl_easy_setopt(handle_.get(), CURLOPT_READFUNCTION,
		    call_function);
		curl_easy_setopt(handle_.get(), CURLOPT_READDATA, p);
	}
}

void request::setup_response_body_to_callback(void* p)
{
	curl_easy_setopt(handle_.get(), CURLOPT_WRITEFUNCTION, call_function);
	curl_easy_setopt(handle_.get(), CURLOPT_WRITEDATA, p);
}

inline
void setup_response_headers(CURL* handle, void* p)
{
	curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, fill_headers);
	curl_easy_setopt(handle, CURLOPT_HEADERDATA, p);
}

template <typename T, size_t N>
inline
T* choose_buffer(T (&arr)[N], std::unique_ptr<T[]>& darr, size_t n)
{
	if (n <= N)
		return arr;
	else
	{
		darr.reset(new T[n]);

		return darr.get();
	}
}

void request::perform_on(response& resp)
{
	if (curl_easy_setopt(handle_.get(), CURLOPT_URL, url.data()))
		throw bad_request();

	if (curl_easy_setopt(handle_.get(), CURLOPT_ACCEPT_ENCODING, ""))
		throw bad_request();

	curl_easy_setopt(handle_.get(), CURLOPT_USERAGENT, "httpverbs/0.1");

#if defined(CURLRES_ASYNCH)
	curl_easy_setopt(handle_.get(), CURLOPT_NOSIGNAL, 1L);
#endif

	std::unique_ptr<curl_slist[]> hll;
	curl_slist fhll[16];

	if (not headers.empty())
	{
		auto buf = choose_buffer(fhll, hll, headers.size());
		auto p = buf;

		for (auto it = begin(headers); it != end(headers); ++it)
		{
			// libcurl modifies and only modifies the input
			// data when your header is in the
			// "header-ended-by;" format; fortunately such
			// headers are already skipped.
			p->data = const_cast<char*>(it->data());
			p->next = p + 1;
			++p;
		}
		(p - 1)->next = nullptr;

		curl_easy_setopt(handle_.get(), CURLOPT_HTTPHEADER, buf);
	}

	headers_parser_stack sk = { false, resp.headers };
	setup_response_headers(handle_.get(), &sk);

	auto r = pooled_perform(handle_.get());

	if (r != CURLE_OK)
		throw bad_response(r);

	long http_code;
	curl_easy_getinfo(handle_.get(), CURLINFO_RESPONSE_CODE, &http_code);
	resp.status_code = int(http_code);

	char* new_url;
	curl_easy_getinfo(handle_.get(), CURLINFO_EFFECTIVE_URL, &new_url);
	resp.url = new_url;
}

size_t read_string(char* to, size_t sz, size_t nmemb, void* from)
{
	auto& sv = *reinterpret_cast<_mini_string_ref*>(from);

	auto copied_size = sv.copy(to, sz * nmemb);
	sv.remove_prefix(copied_size);

	return copied_size;
}

size_t write_string(char* from, size_t sz, size_t nmemb, void* to)
{
	auto& s = *reinterpret_cast<std::string*>(to);

	s.append(from, sz * nmemb);

	return sz * nmemb;
}

size_t call_function(char* from, size_t sz, size_t nmemb, void* f)
{
	return (*reinterpret_cast<request::callback_t*>(f))(from, sz * nmemb);
}

size_t fill_headers(char* from, size_t sz, size_t nmemb, void* to)
{
	auto& sk = *reinterpret_cast<headers_parser_stack*>(to);

	BOOST_ASSERT_MSG(sz * nmemb >= 2, "libcurl returns malformed header");

	// discard headers from the last response, if any, and skip
	// the first line
	if (not sk.done_status_line)
	{
		sk.done_status_line = true;
		sk.ls.clear();
	}
	else
	{
		auto size_without_CR_LF = sz * nmemb - 2;

		if (size_without_CR_LF == 0)
			sk.done_status_line = false;
		else
			sk.ls.add(std::string(from, size_without_CR_LF));
	}

	return sz * nmemb;
}

}
