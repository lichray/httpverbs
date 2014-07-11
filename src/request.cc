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
#include <httpverbs/response.h>
#include <httpverbs/exceptions.h>

#include <curl/curl.h>

#include <type_traits>
#include <limits>
#include <stdlib.h>
#include <new>
#include <stdexcept>
#include "stdex/string_view.h"

namespace httpverbs
{

void request::_char_deleter::operator()(char* p) const
{
	free(p);
}

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
	bool done_final_header_line;
	header_dict& ls;
};

}

request::request(char const* method, std::string url) :
	url(std::move(url)),
	handle_(curl_easy_init())
{
	if (handle_ == nullptr)
		throw bad_request();

	curl_easy_setopt(handle_.get(), CURLOPT_CUSTOMREQUEST, method);
}

request::request(request&& other) :
	url(std::move(other.url)),
	headers(std::move(other.headers)),
	data(std::move(other.data)),
	handle_(std::move(other.handle_))
{}

void swap(request& a, request& b)
{
	using std::swap;

	swap(a.url, b.url);
	swap(a.headers, b.headers);
	swap(a.data, b.data);
	swap(a.handle_, b.handle_);
}

response request::perform()
{
	stdex::string_view sv = data;
	setup_request_body_from_bytes(&sv, of_length(sv.size()));

	response resp;
	setup_response_body_to_string(&resp.content);

	perform_on(resp);

	return resp;
}

of_length::of_length(size_t n) :
	v_(n <= std::make_unsigned<curl_off_t>::type(
	    std::numeric_limits<curl_off_t>::max()) ?
	    n :
	    throw std::length_error("larger then curl_off_t"))
{}

response request::perform(callback_t reader, of_length n, callback_t writer)
{
	response resp;
	setup_request_body_from_callback(&reader, n);
	setup_response_body_to_callback(&writer);

	perform_on(resp);

	return resp;
}

response request::perform(callback_t reader, of_length n, to_content_t)
{
	response resp;
	setup_request_body_from_callback(&reader, n);
	setup_response_body_to_string(&resp.content);

	perform_on(resp);

	return resp;
}

response request::perform(callback_t reader, of_length n,
    ignoring_response_body_t)
{
	response resp;
	setup_request_body_from_callback(&reader, n);
	setup_response_body_ignored();

	perform_on(resp);

	return resp;
}

response request::perform(from_data_t, callback_t writer)
{
	stdex::string_view sv = data;
	setup_request_body_from_bytes(&sv, of_length(sv.size()));

	response resp;
	setup_response_body_to_callback(&writer);

	perform_on(resp);

	return resp;
}

response request::perform(from_data_t, to_content_t)
{
	return perform();
}

response request::perform(from_data_t, ignoring_response_body_t)
{
	stdex::string_view sv = data;
	setup_request_body_from_bytes(&sv, of_length(sv.size()));

	response resp;
	setup_response_body_ignored();

	perform_on(resp);

	return resp;
}

void request::setup_request_body_from_bytes(void* p, of_length n)
{
	auto sz = curl_off_t(n.value());

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
	curl_easy_setopt(handle_.get(), CURLOPT_WRITEFUNCTION, write_string);
	curl_easy_setopt(handle_.get(), CURLOPT_WRITEDATA, p);
}

void request::setup_request_body_from_callback(void* p, of_length n)
{
	auto sz = curl_off_t(n.value());

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

void request::setup_response_body_ignored()
{
	curl_easy_setopt(handle_.get(), CURLOPT_NOBODY, 1L);
}

void request::setup_response_headers(void* p)
{
	curl_easy_setopt(handle_.get(), CURLOPT_HEADERFUNCTION, fill_headers);
	curl_easy_setopt(handle_.get(), CURLOPT_HEADERDATA, p);
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
	curl_easy_setopt(handle_.get(), CURLOPT_URL, url.data());
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

	headers_parser_stack sk = { false, false, resp.headers };
	setup_response_headers(&sk);

	auto r = curl_easy_perform(handle_.get());

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
	auto& sv = *reinterpret_cast<stdex::string_view*>(from);

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

	if (sz * nmemb < 2)
		throw std::logic_error("libcurl returns malformed header");

	// skip the first line
	if (not sk.done_status_line)
	{
		sk.done_status_line = true;
	}
	// skip the other embedded headers
	else if (not sk.done_final_header_line)
	{
		auto size_without_CR_LF = sz * nmemb - 2;

		if (size_without_CR_LF == 0)
			sk.done_final_header_line = true;
		else
			sk.ls.add(std::string(from, size_without_CR_LF));
	}

	return sz * nmemb;
}

}
