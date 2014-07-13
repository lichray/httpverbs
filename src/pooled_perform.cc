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

#include <httpverbs/exceptions.h>

#include "pooled_perform.h"

#include <memory>
#include "stdex/defer.h"

namespace httpverbs
{

struct _curl_share_deleter
{
	void operator()(CURLSH* p) const
	{
		curl_share_cleanup(p);
	}
};

static
CURLSH* share_handle()
{
	static
	std::unique_ptr<CURLSH, _curl_share_deleter> handle([]
	    {
		auto p = curl_share_init();

		if (p == nullptr)
			throw bad_connection_pool();

		if (curl_share_setopt(p, CURLSHOPT_SHARE,
		    CURL_LOCK_DATA_SSL_SESSION))
			throw bad_connection_pool();

		return p;
	    }());

	return handle.get();
}

CURLcode pooled_perform(CURL* handle)
{
	auto ssl_cache = share_handle();

	curl_easy_setopt(handle, CURLOPT_SHARE, ssl_cache);
	defer(curl_easy_setopt(handle, CURLOPT_SHARE, nullptr));

	return curl_easy_perform(handle);
}

}
