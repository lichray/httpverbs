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

#ifndef HTTPVERBS_C__FILE_H
#define HTTPVERBS_C__FILE_H

#include <cstdio>
#include <functional>

namespace httpverbs
{
namespace keywords
{

auto from_c_file(FILE* stream)
	-> decltype(std::bind(
	    fread,
	    std::placeholders::_1,
	    1,
	    std::placeholders::_2,
	    stream))
{
	using namespace std::placeholders;

	return std::bind(fread, _1, 1, _2, stream);
}

auto to_c_file(FILE* stream)
	-> decltype(std::bind(
	    fwrite,
	    std::placeholders::_1,
	    1,
	    std::placeholders::_2,
	    stream))
{
	using namespace std::placeholders;

	return std::bind(fwrite, _1, 1, _2, stream);
}

}
}

#endif
