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

#ifndef HTTPVERBS_RESPONSE_H
#define HTTPVERBS_RESPONSE_H

#include <string>
#include <vector>

namespace httpverbs
{

struct response
{
	std::string url;
	std::string content;
	int status_code;

	explicit response(int status_code) :
		status_code(status_code)
	{}

	friend inline
	bool operator==(response const& a, response const& b)
	{
		return a.url == b.url and
		    a.content == b.content and
		    a.status_code == b.status_code and
		    a.headers_ == b.headers_;
	}

	friend inline
	bool operator!=(response const& a, response const& b)
	{
		return !(a == b);
	}

	bool ok() const
	{
		return status_code < 400 or status_code >= 600;
	}

	std::string get_header(char const* name) const;

private:
	friend struct request;
	response() {}  // status_code code has an indeterminate value

	std::vector<std::string> headers_;
};

}

#endif
