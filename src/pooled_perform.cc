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
#include "config.h"

#if defined(USE_BOOST_CHRONO)
#define BOOST_CHRONO_HEADER_ONLY
#include <boost/chrono.hpp>
#else
#include <chrono>
#endif

#if defined(USE_BOOST_TSS)
#include <boost/thread/tss.hpp>
#endif

#if defined(WIN32)
#include <windows.h>
#else
#include <poll.h>
#endif

#include <memory>
#include "stdex/defer.h"

#if !defined(USE_BOOST_TSS)

# if defined(PER_THREAD_CACHE)
# define TSS_ thread_local
# else
# define TSS_ static
# endif

#define TSS_POINTER(type, name, cleanup, init)			\
	struct type##_deleter					\
	{							\
		void operator()(type* p) const			\
		{						\
			cleanup(p);				\
		}						\
	};							\
	TSS_ std::unique_ptr<type, type##_deleter> name(init);

#else

// cast to char* to workaround a bug in Boost
// https://svn.boost.org/trac/boost/ticket/10196
#define TSS_POINTER(type, name, cleanup, init)			\
	static boost::thread_specific_ptr<char> name(		\
	    [](char* p) { cleanup((type*)p); });		\
	if (name.get() == nullptr)				\
		name.reset(reinterpret_cast<char*>(init));

#endif

namespace httpverbs
{

static CURLcode do_transfer(CURLM*);

static
CURLSH* share_handle()
{
	TSS_POINTER(CURLSH, handle, curl_share_cleanup, []
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

static
CURLM* multi_handle()
{
	TSS_POINTER(CURLM, handle, curl_multi_cleanup, []
	    {
		auto p = curl_multi_init();

		if (p == nullptr)
			throw bad_connection_pool();

		// libcurl hard-coded ssl session cache to 8, so it
		// doesn't help much to cache more connections
		if (curl_multi_setopt(p, CURLMOPT_MAXCONNECTS, 8L))
			throw bad_connection_pool();

		return p;
	    }());

	return handle.get();
}

CURLcode pooled_perform(CURL* handle)
{
	// libcurl tries to handle SIGPIPE internally no matter whether
	// CURLOPT_NOSIGNAL is set.  Hope it's not a big deal if we
	// unconditionally do not block SIGPIPE here.

	auto ssl_cache = share_handle();
	auto conn_cache = multi_handle();

	curl_easy_setopt(handle, CURLOPT_SHARE, ssl_cache);
	defer(curl_easy_setopt(handle, CURLOPT_SHARE, nullptr));

	if (curl_multi_add_handle(conn_cache, handle))
		throw bad_connection_pool();

	defer(curl_multi_remove_handle(conn_cache, handle));

	return do_transfer(conn_cache);
}

#if defined(USE_BOOST_CHRONO)
using namespace boost::chrono;
#else
using namespace std::chrono;
#endif

template <typename Rep, typename Period>
inline
void wait_for(duration<Rep,Period> const& d)
{
	auto ms = duration_cast<milliseconds>(d);

#if defined(WIN32)
	Sleep(ms.count());
#else
	// we have turned on CURL_GLOBAL_ACK_EINTR, so the call
	// is allowed to return early
	poll(nullptr, 0, ms.count());
#endif
}

// The code is modified from libcurl's `easy_transfer` function
// in lib/easy.c, with less states using returns.
CURLcode do_transfer(CURLM* multi)
{
	int without_fds = 0;
	int still_running;

	do
	{
		int ret;
		auto before = high_resolution_clock::now();

		if (curl_multi_wait(multi, nullptr, 0, 1000, &ret))
			return CURLE_OUT_OF_MEMORY;

		if (ret == -1)
		{
			return CURLE_RECV_ERROR;
		}
		else if (ret == 0)
		{
			auto after = high_resolution_clock::now();

			if ((after - before) <= milliseconds(10))
			{
				++without_fds;

				if (without_fds > 2)
				{
					if (without_fds < 10)
						wait_for(milliseconds(
						    1 << (without_fds - 1)));
					else
						wait_for(seconds(1));
				}
			}
			else
				without_fds = 0;
		}
		else
			without_fds = 0;

		if (curl_multi_perform(multi, &still_running))
			return CURLE_OUT_OF_MEMORY;

	} while (still_running);

	int rc;
	auto msg = curl_multi_info_read(multi, &rc);

	if (msg != nullptr)
		return msg->data.result;
	else
		return CURLE_OK;
}

}
