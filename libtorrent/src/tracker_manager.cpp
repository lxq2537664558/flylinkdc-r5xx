/*

Copyright (c) 2003-2016, Arvid Norberg
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the distribution.
    * Neither the name of the author nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#include <utility>
#include <vector>
#include <cctype>
#include <functional>

#include "libtorrent/tracker_manager.hpp"
#include "libtorrent/http_tracker_connection.hpp"
#include "libtorrent/udp_tracker_connection.hpp"
#include "libtorrent/aux_/session_impl.hpp"
#include "libtorrent/aux_/io.hpp"
#include "libtorrent/span.hpp"


using namespace std::placeholders;

namespace
{
	enum
	{
		minimum_tracker_response_length = 3,
		http_buffer_size = 2048
	};

}

namespace libtorrent
{
	using namespace libtorrent::aux;

	timeout_handler::timeout_handler(io_service& ios)
		: m_completion_timeout(0)
		, m_start_time(clock_type::now())
		, m_read_time(m_start_time)
		, m_timeout(ios)
		, m_read_timeout(0)
		, m_abort(false)
	{}

	void timeout_handler::set_timeout(int completion_timeout, int read_timeout)
	{
		m_completion_timeout = completion_timeout;
		m_read_timeout = read_timeout;
		m_start_time = m_read_time = clock_type::now();

		TORRENT_ASSERT(completion_timeout > 0 || read_timeout > 0);

		if (m_abort) return;

		int timeout = 0;
		if (m_read_timeout > 0) timeout = m_read_timeout;
		if (m_completion_timeout > 0)
		{
			timeout = timeout == 0
				? m_completion_timeout
				: (std::min)(m_completion_timeout, timeout);
		}

		ADD_OUTSTANDING_ASYNC("timeout_handler::timeout_callback");
		error_code ec;
		m_timeout.expires_at(m_read_time + seconds(timeout), ec);
		m_timeout.async_wait(std::bind(
			&timeout_handler::timeout_callback, shared_from_this(), _1));
	}

	void timeout_handler::restart_read_timeout()
	{
		m_read_time = clock_type::now();
	}

	void timeout_handler::cancel()
	{
		m_abort = true;
		m_completion_timeout = 0;
		error_code ec;
		m_timeout.cancel(ec);
	}

	void timeout_handler::timeout_callback(error_code const& error)
	{
		COMPLETE_ASYNC("timeout_handler::timeout_callback");
		if (m_abort) return;

		time_point now = clock_type::now();
		time_duration receive_timeout = now - m_read_time;
		time_duration completion_timeout = now - m_start_time;

		if ((m_read_timeout
				&& m_read_timeout <= total_seconds(receive_timeout))
			|| (m_completion_timeout
				&& m_completion_timeout <= total_seconds(completion_timeout))
			|| error)
		{
			on_timeout(error);
			return;
		}

		int timeout = 0;
		if (m_read_timeout > 0) timeout = m_read_timeout;
		if (m_completion_timeout > 0)
		{
			timeout = timeout == 0
				? int(m_completion_timeout - total_seconds(m_read_time - m_start_time))
				: (std::min)(int(m_completion_timeout - total_seconds(m_read_time - m_start_time)), timeout);
		}
		ADD_OUTSTANDING_ASYNC("timeout_handler::timeout_callback");
		error_code ec;
		m_timeout.expires_at(m_read_time + seconds(timeout), ec);
		m_timeout.async_wait(
			std::bind(&timeout_handler::timeout_callback, shared_from_this(), _1));
	}

	tracker_connection::tracker_connection(
		tracker_manager& man
		, tracker_request const& req
		, io_service& ios
		, std::weak_ptr<request_callback> r)
		: timeout_handler(ios)
		, m_req(req)
		, m_requester(std::move(r))
		, m_man(man)
	{}

	std::shared_ptr<request_callback> tracker_connection::requester() const
	{
		return m_requester.lock();
	}

	void tracker_connection::fail(error_code const& ec, int code
		, char const* msg, int interval, int min_interval)
	{
		// we need to post the error to avoid deadlock
			get_io_service().post(std::bind(&tracker_connection::fail_impl
					, shared_from_this(), ec, code, std::string(msg), interval, min_interval));
	}

	void tracker_connection::fail_impl(error_code const& ec, int code
		, std::string msg, int interval, int min_interval)
	{
		std::shared_ptr<request_callback> cb = requester();
		if (cb) cb->tracker_request_error(m_req, code, ec, msg.c_str()
			, interval == 0 ? min_interval : interval);
		close();
	}

	void tracker_connection::sent_bytes(int bytes)
	{
		m_man.sent_bytes(bytes);
	}

	void tracker_connection::received_bytes(int bytes)
	{
		m_man.received_bytes(bytes);
	}

	void tracker_connection::close()
	{
		cancel();
		m_man.remove_request(this);
	}

	tracker_manager::tracker_manager(send_fun_t const& send_fun
		, send_fun_hostname_t const& send_fun_hostname
		, counters& stats_counters
		, resolver_interface& resolver
		, aux::session_settings const& sett
#if !defined TORRENT_DISABLE_LOGGING || TORRENT_USE_ASSERTS
		, aux::session_logger& ses
#endif
		)
		: m_send_fun(send_fun)
		, m_send_fun_hostname(send_fun_hostname)
		, m_host_resolver(resolver)
		, m_settings(sett)
		, m_stats_counters(stats_counters)
#if !defined TORRENT_DISABLE_LOGGING || TORRENT_USE_ASSERTS
		, m_ses(ses)
#endif
		, m_abort(false)
	{}

	tracker_manager::~tracker_manager()
	{
		TORRENT_ASSERT(m_abort);
		abort_all_requests(true);
	}

	void tracker_manager::sent_bytes(int bytes)
	{
		TORRENT_ASSERT(m_ses.is_single_thread());
		m_stats_counters.inc_stats_counter(counters::sent_tracker_bytes, bytes);
	}

	void tracker_manager::received_bytes(int bytes)
	{
		TORRENT_ASSERT(m_ses.is_single_thread());
		m_stats_counters.inc_stats_counter(counters::recv_tracker_bytes, bytes);
	}

	void tracker_manager::remove_request(tracker_connection const* c)
	{
		TORRENT_ASSERT(is_single_thread());
		http_conns_t::iterator i = std::find_if(m_http_conns.begin(), m_http_conns.end()
			, [c] (std::shared_ptr<http_tracker_connection> const& ptr) { return ptr.get() == c; });
		if (i != m_http_conns.end())
		{
			m_http_conns.erase(i);
			return;
		}

		udp_conns_t::iterator j = std::find_if(m_udp_conns.begin(), m_udp_conns.end()
			, [c] (udp_conns_t::value_type const& uc) { return uc.second.get() == c; });
		if (j != m_udp_conns.end())
		{
			m_udp_conns.erase(j);
			return;
		}
	}

	void tracker_manager::update_transaction_id(
		std::shared_ptr<udp_tracker_connection> c
		, std::uint64_t tid)
	{
		TORRENT_ASSERT(is_single_thread());
		m_udp_conns.erase(c->transaction_id());
		m_udp_conns[tid] = c;
	}

	void tracker_manager::queue_request(
		io_service& ios
		, tracker_request req
		, std::weak_ptr<request_callback> c)
	{
		TORRENT_ASSERT(is_single_thread());
		TORRENT_ASSERT(req.num_want >= 0);
		TORRENT_ASSERT(!m_abort || req.event == tracker_request::stopped);
		if (m_abort && req.event != tracker_request::stopped) return;
		if (req.event == tracker_request::stopped)
			req.num_want = 0;

		TORRENT_ASSERT(!m_abort || req.event == tracker_request::stopped);
		if (m_abort && req.event != tracker_request::stopped)
			return;

		std::string protocol = req.url.substr(0, req.url.find(':'));

#ifdef TORRENT_USE_OPENSSL
		if (protocol == "http" || protocol == "https")
#else
		if (protocol == "http")
#endif
		{
			std::shared_ptr<http_tracker_connection> con
				= std::make_shared<http_tracker_connection>(
					std::ref(ios), std::ref(*this), std::cref(req), c);
			m_http_conns.push_back(con);
			con->start();
			return;
		}
		else if (protocol == "udp")
		{
			std::shared_ptr<udp_tracker_connection> con
				= std::make_shared<udp_tracker_connection>(
					std::ref(ios), std::ref(*this), std::cref(req) , c);
			m_udp_conns[con->transaction_id()] = con;
			con->start();
			return;
		}

		// we need to post the error to avoid deadlock
		if (std::shared_ptr<request_callback> r = c.lock())
			ios.post(std::bind(&request_callback::tracker_request_error, r, req
				, -1, error_code(errors::unsupported_url_protocol)
				, "", 0));
	}

	bool tracker_manager::incoming_packet(udp::endpoint const& ep
		, span<char const> const buf)
	{
		TORRENT_ASSERT(is_single_thread());
		// ignore packets smaller than 8 bytes
		if (buf.size() < 8)
		{
#ifndef TORRENT_DISABLE_LOGGING
			if (m_ses.should_log())
			{
				m_ses.session_log("incoming packet from %s, not a UDP tracker message "
					"(%d Bytes)", print_endpoint(ep).c_str(), int(buf.size()));
			}
#endif
			return false;
		}

		// the first word is the action, if it's not [0, 3]
		// it's not a valid udp tracker response
		span<const char> ptr = buf;
		std::uint32_t const action = aux::read_uint32(ptr);
		if (action > 3) return false;

		std::uint32_t const transaction = aux::read_uint32(ptr);
		udp_conns_t::iterator const i = m_udp_conns.find(transaction);

		if (i == m_udp_conns.end())
		{
#ifndef TORRENT_DISABLE_LOGGING
			if (m_ses.should_log())
			{
				m_ses.session_log("incoming UDP tracker packet from %s has invalid "
					"transaction ID (%x)", print_endpoint(ep).c_str()
					, transaction);
			}
#endif
			return false;
		}

		std::shared_ptr<udp_tracker_connection> const p = i->second;
		// on_receive() may remove the tracker connection from the list
		return p->on_receive(ep, buf);
	}

	void tracker_manager::incoming_error(error_code const&
		, udp::endpoint const&)
	{
		TORRENT_ASSERT(is_single_thread());
		// TODO: 2 implement
	}

	bool tracker_manager::incoming_packet(char const* hostname
		, span<char const> const buf)
	{
		TORRENT_ASSERT(is_single_thread());
		// ignore packets smaller than 8 bytes
		if (buf.size() < 16) return false;

		// the first word is the action, if it's not [0, 3]
		// it's not a valid udp tracker response
		span<const char> ptr = buf;
		std::uint32_t const action = aux::read_uint32(ptr);
		if (action > 3) return false;

		std::uint32_t const transaction = aux::read_uint32(ptr);
		udp_conns_t::iterator const i = m_udp_conns.find(transaction);

		if (i == m_udp_conns.end())
		{
#ifndef TORRENT_DISABLE_LOGGING
			// now, this may not have been meant to be a tracker response,
			// but chances are pretty good, so it's probably worth logging
			m_ses.session_log("incoming UDP tracker packet from %s has invalid "
				"transaction ID (%x)", hostname, int(transaction));
#endif
			return false;
		}

		std::shared_ptr<udp_tracker_connection> const p = i->second;
		// on_receive() may remove the tracker connection from the list
		return p->on_receive_hostname(hostname, buf);
	}

	void tracker_manager::send_hostname(char const* hostname, int const port
		, span<char const> p, error_code& ec, int const flags)
	{
		TORRENT_ASSERT(is_single_thread());
		m_send_fun_hostname(hostname, port, p, ec, flags);
	}

	void tracker_manager::send(udp::endpoint const& ep
		, span<char const> p
		, error_code& ec, int const flags)
	{
		TORRENT_ASSERT(is_single_thread());
		m_send_fun(ep, p, ec, flags);
	}

	void tracker_manager::abort_all_requests(bool all)
	{
		// this is called from the destructor too, which is not subject to the
		// single-thread requirement.
		TORRENT_ASSERT(all || is_single_thread());
		// removes all connections except 'event=stopped'-requests

		m_abort = true;
		http_conns_t close_http_connections;
		std::vector<std::shared_ptr<udp_tracker_connection>> close_udp_connections;

		for (http_conns_t::iterator i = m_http_conns.begin()
			, end(m_http_conns.end()); i != end; ++i)
		{
			http_tracker_connection* c = i->get();
			tracker_request const& req = c->tracker_req();
			if (req.event == tracker_request::stopped && !all)
				continue;

			close_http_connections.push_back(*i);

#ifndef TORRENT_DISABLE_LOGGING
			std::shared_ptr<request_callback> rc = c->requester();
			if (rc) rc->debug_log("aborting: %s", req.url.c_str());
#endif
		}
		for (udp_conns_t::iterator i = m_udp_conns.begin()
			, end(m_udp_conns.end()); i != end; ++i)
		{
			std::shared_ptr<udp_tracker_connection> c = i->second;
			tracker_request const& req = c->tracker_req();
			if (req.event == tracker_request::stopped && !all)
				continue;

			close_udp_connections.push_back(c);

#ifndef TORRENT_DISABLE_LOGGING
			std::shared_ptr<request_callback> rc = c->requester();
			if (rc) rc->debug_log("aborting: %s", req.url.c_str());
#endif
		}

		for (http_conns_t::iterator i = close_http_connections.begin()
			, end(close_http_connections.end()); i != end; ++i)
		{
			(*i)->close();
		}

		for (auto const& c : close_udp_connections)
		{
			c->close();
		}
	}

	bool tracker_manager::empty() const
	{
		TORRENT_ASSERT(is_single_thread());
		return m_http_conns.empty() && m_udp_conns.empty();
	}

	int tracker_manager::num_requests() const
	{
		TORRENT_ASSERT(is_single_thread());
		return int(m_http_conns.size() + m_udp_conns.size());
	}
}