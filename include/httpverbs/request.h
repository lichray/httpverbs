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

#include <string>
#include <memory>

extern "C"
{
struct curl_slist;
}

namespace httpverbs
{

struct response;

struct request
{
	std::string url;
	std::string data;

	request(char const* method, std::string url);

	friend inline
	bool operator==(request const& a, request const& b)
	{
		return a.handle_ == b.handle_;
	}

	friend inline
	bool operator!=(request const& a, request const& b)
	{
		return !(a == b);
	}

	void add_header(char const* name, char const* value);
	void refuse_body();
	response perform();

private:
	void setup_request_body_from_bytes(void* p, size_t sz);
	void setup_response_body_to_string(void* p);
	void setup_sorted_response_headers(void* p);
	void perform_on(response& resp);

	struct _curl_slist_deleter
	{
		void operator()(curl_slist*) const;
	};

	struct _curl_handle_deleter
	{
		void operator()(void*) const;
	};

	std::string header_buffer_;
	std::unique_ptr<curl_slist, _curl_slist_deleter> headers_;
	std::unique_ptr<void, _curl_handle_deleter> handle_;
};

}

#endif
