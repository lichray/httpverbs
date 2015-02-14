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

#ifndef HTTPVERBS_STREAM_H
#define HTTPVERBS_STREAM_H

#include <istream>
#include <ostream>

namespace httpverbs
{

struct _request_sgetn_cb
{
	explicit _request_sgetn_cb(std::streambuf* p) : p_(p)
	{}

	size_t operator()(char* dst, size_t sz)
	{
		return static_cast<size_t>(p_->sgetn(dst, sz));
	}

private:
	std::streambuf* p_;
};

struct _request_sputn_cb
{
	explicit _request_sputn_cb(std::streambuf* p) : p_(p)
	{}

	size_t operator()(char const* dst, size_t sz)
	{
		return static_cast<size_t>(p_->sputn(dst, sz));
	}

private:
	std::streambuf* p_;
};

namespace keywords
{

inline
auto from_stream(std::istream& is)
	-> _request_sgetn_cb
{
	return _request_sgetn_cb(is.rdbuf());
}

inline
auto to_stream(std::ostream& os)
	-> _request_sputn_cb
{
	return _request_sputn_cb(os.rdbuf());
}

}

}

#endif
