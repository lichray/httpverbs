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

namespace httpverbs
{

request::request(char const* method, std::string url) :
	url(std::move(url)),
	headers_(nullptr, curl_slist_free_all),
	handle_(curl_easy_init(), curl_easy_cleanup)
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

response request::perform()
{
	curl_easy_setopt(handle_.get(), CURLOPT_URL, url.data());

	if (headers_ != nullptr)
		curl_easy_setopt(handle_.get(), CURLOPT_HTTPHEADER,
		    headers_.get());

	auto r = curl_easy_perform(handle_.get());

	if (r != CURLE_OK)
		throw bad_response(r);

	long http_code;
	curl_easy_getinfo(handle_.get(), CURLINFO_RESPONSE_CODE, &http_code);

	return response(int(http_code));
}

}
