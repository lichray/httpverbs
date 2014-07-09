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

#include "header_algo.h"

namespace httpverbs
{

void request::_char_deleter::operator()(char* p) const
{
	free(p);
}

void request::_curl_slist_deleter::operator()(curl_slist* p) const
{
	curl_slist_free_all(p);
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
	std::vector<std::string>& ls;
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
	data(std::move(other.data)),
	headers_(std::move(other.headers_)),
	handle_(std::move(other.handle_))
{}

void swap(request& a, request& b)
{
	using std::swap;

	swap(a.url, b.url);
	swap(a.data, b.data);
	swap(a.headers_, b.headers_);
	swap(a.handle_, b.handle_);
}

void request::add_header(char const* name, char const* value)
{
	char buf[128];
	auto nv = strlen(name);
	auto lv = strlen(value);
	char* p;

	if (lv > 0)
	{
		p = try_local_buffer(buf, nv + lv + 3);
		_mscpy(_mscpy(_mscpy(p, name, nv), ": ", 2), value, lv + 1);
		add_curl_header(p);
	}
#if LIBCURL_VERSION_NUM >= 0x071700
	else  // empty header support in 7.23.0
	{
		p = try_local_buffer(buf, nv + 2);
		_mscpy(_mscpy(p, name, nv), ";", 2);
		add_curl_header(p);
	}
#endif
}

void request::add_curl_header(char const* line)
{
	headers_.reset(curl_slist_append(headers_.release(), line));
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

void request::setup_sorted_response_headers(void* p)
{
	curl_easy_setopt(handle_.get(), CURLOPT_HEADERFUNCTION, fill_headers);
	curl_easy_setopt(handle_.get(), CURLOPT_HEADERDATA, p);
}

void request::perform_on(response& resp)
{
	curl_easy_setopt(handle_.get(), CURLOPT_URL, url.data());
	curl_easy_setopt(handle_.get(), CURLOPT_USERAGENT, "httpverbs/0.1");

#if defined(CURLRES_ASYNCH)
	curl_easy_setopt(handle_.get(), CURLOPT_NOSIGNAL, 1L);
#endif

	if (headers_ != nullptr)
		curl_easy_setopt(handle_.get(), CURLOPT_HTTPHEADER,
		    headers_.get());

	headers_parser_stack sk = { false, false, resp.headers_ };
	setup_sorted_response_headers(&sk);

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

template <size_t N>
char* request::try_local_buffer(char (&arr)[N], size_t sz)
{
	if (sz <= N)
		return arr;
	else
	{
		// reallocf(3)
		auto oldp = header_buffer_.release();
		header_buffer_.reset(
		    reinterpret_cast<char*>(realloc(oldp, sz)));

		if (header_buffer_.get() == nullptr)
		{
			free(oldp);
			throw std::bad_alloc();
		}

		return header_buffer_.get();
	}
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
		{
			sk.done_final_header_line = true;
		}
		else
		{
			auto p = strchr(from, ':');

			if (*p == '\0')
				goto done;

			auto it = header_position(sk.ls, from, p - from);

#if !defined(_MSC_VER) || _MSC_VER >= 1700
			sk.ls.emplace(it, from, size_without_CR_LF);
#else
			sk.ls.insert(it, std::string(from,
			    size_without_CR_LF));
#endif
		}
	}

done:
	return sz * nmemb;
}

}
