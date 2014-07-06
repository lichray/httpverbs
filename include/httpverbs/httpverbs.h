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

#ifndef HTTPVERBS_HTTPVERBS_H
#define HTTPVERBS_HTTPVERBS_H

#include "enable_library.h"
#include "request.h"
#include "response.h"

namespace httpverbs
{

inline
response get(std::string url)
{
	return request("GET", std::move(url)).perform();
}

inline
response put(std::string url)
{
	return request("PUT", std::move(url)).perform();
}

inline
response put(std::string url, std::string data)
{
	auto req = request("PUT", std::move(url));
	req.data = std::move(data);

	return req.perform();
}

inline
response post(std::string url)
{
	return request("POST", std::move(url)).perform();
}

inline
response post(std::string url, std::string data)
{
	auto req = request("POST", std::move(url));
	req.data = std::move(data);

	return req.perform();
}

inline
response delete_(std::string url)
{
	return request("DELETE", std::move(url)).perform();
}

inline
response head(std::string url)
{
	auto req = request("HEAD", std::move(url));
	req.ignore_response_body();

	return req.perform();
}

inline
response options(std::string url)
{
	return request("OPTIONS", std::move(url)).perform();
}

inline
response patch(std::string url)
{
	return request("PATCH", std::move(url)).perform();
}

inline
response patch(std::string url, std::string data)
{
	auto req = request("PATCH", std::move(url));
	req.data = std::move(data);

	return req.perform();
}

}

#endif
