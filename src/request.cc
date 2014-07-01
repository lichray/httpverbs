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
#include <stdexcept>
#include <algorithm>
#include <strings.h>
#include "stdex/string_view.h"

namespace httpverbs
{

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

	curl_easy_setopt(handle_.get(), CURLOPT_USERAGENT, "httpverbs/0.1");
	curl_easy_setopt(handle_.get(), CURLOPT_CUSTOMREQUEST, method);
}

void request::add_header(char const* name, char const* value)
{
	header_buffer_.assign(name).append(": ").append(value);
	headers_.reset(curl_slist_append(headers_.get(),
	    header_buffer_.data()));
}

void request::refuse_body()
{
	curl_easy_setopt(handle_.get(), CURLOPT_NOBODY, 1L);
}

response request::perform()
{
	stdex::string_view sv = data;
	setup_request_body_from_bytes(&sv, sv.size());

	response resp;
	setup_response_body_to_string(&resp.content);

	perform_on(resp);

	return resp;
}

void request::setup_request_body_from_bytes(void* p, size_t sz)
{
	if (sz > std::make_unsigned<curl_off_t>::type(
	    std::numeric_limits<curl_off_t>::max()))
		throw std::length_error("request::data");

	if (sz != 0)
	{
		curl_easy_setopt(handle_.get(), CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(handle_.get(), CURLOPT_READFUNCTION,
		    read_string);
		curl_easy_setopt(handle_.get(), CURLOPT_READDATA, p);
		curl_easy_setopt(handle_.get(), CURLOPT_INFILESIZE_LARGE, sz);
	}
}

void request::setup_response_body_to_string(void* p)
{
	curl_easy_setopt(handle_.get(), CURLOPT_WRITEFUNCTION, write_string);
	curl_easy_setopt(handle_.get(), CURLOPT_WRITEDATA, p);
}

void request::setup_sorted_response_headers(void* p)
{
	curl_easy_setopt(handle_.get(), CURLOPT_HEADERFUNCTION, fill_headers);
	curl_easy_setopt(handle_.get(), CURLOPT_HEADERDATA, p);
}

void request::perform_on(response& resp)
{
	curl_easy_setopt(handle_.get(), CURLOPT_URL, url.data());

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
			// this guarantees the headers to be sorted until
			// the first '\0'. Since field-name is not allowed
			// to contain '\0', this also guarantees the field-
			// names to be sorted.
			auto it = std::lower_bound(begin(sk.ls), end(sk.ls),
			    from, [](std::string const& a, char const* b)
			    {
				return (strcasecmp(a.data(), b) < 0);
			    });

			sk.ls.emplace(it, from, size_without_CR_LF);
		}
	}

	return sz * nmemb;
}

}
