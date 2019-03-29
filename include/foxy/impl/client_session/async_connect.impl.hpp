//
// Copyright (c) 2018-2019 Christian Mazakas (christian dot mazakas at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/LeonineKing1199/foxy
//

#ifndef FOXY_IMPL_CLIENT_SESSION_ASYNC_CONNECT_IMPL_HPP_
#define FOXY_IMPL_CLIENT_SESSION_ASYNC_CONNECT_IMPL_HPP_

#include <foxy/client_session.hpp>
#include <foxy/type_traits.hpp>
#include <boost/beast/core/async_base.hpp>

namespace foxy
{
template <class ConnectHandler>
auto
client_session::async_connect(std::string host, std::string service, ConnectHandler&& handler) & ->
  typename boost::asio::async_result<std::decay_t<ConnectHandler>,
                                     void(boost::system::error_code,
                                          boost::asio::ip::tcp::endpoint)>::return_type
{
  using handler_type = typename boost::asio::async_completion<
    ConnectHandler,
    void(boost::system::error_code, boost::asio::ip::tcp::endpoint)>::completion_handler_type;

  using base_type = boost::beast::stable_async_base<handler_type, executor_type>;

  struct op : base_type, boost::asio::coroutine
  {
    struct state
    {
      std::string                                  host;
      std::string                                  service;
      boost::asio::ip::tcp::resolver               resolver;
      boost::asio::ip::tcp::resolver::results_type results;
      boost::asio::ip::tcp::endpoint               endpoint;

      explicit state(std::string host_, std::string service_)
        : host(std::move(host_))
        , service(std::move(service_))
        , resolver(session.stream.get_executor().context())
      {
      }
    };

    ::foxy::session& session;
    state&           s;

    op(::foxy::session& session_, std::string host_, std::string service_, handler_type& handler)
      : base_type(std::move(handler), session_.get_executor())
      , session(session_)
      , s(boost::beast::allocate_stable<state>(*this, std::move(host_), std::move(service_)))
    {
    }

    struct on_resolve_t
    {
    };
    struct on_connect_t
    {
    };

    auto
    operator()(on_resolve_t,
               boost::system::error_code                    ec,
               boost::asio::ip::tcp::resolver::results_type results) -> void
    {
      s.results = std::move(results);
      (*this)(ec, 0);
    }

    auto
    operator()(on_connect_t, boost::system::error_code ec, boost::asio::ip::tcp::endpoint endpoint)
      -> void
    {
      s.endpoint = std::move(endpoint);
      (*this)(ec, 0);
    }

    auto
    operator()(boost::system::error_code ec,
               std::size_t const         bytes_transferred,
               bool const                is_continuation) -> void
    {
      BOOST_ASIO_CORO_REENTER(*this)
      {
        if (s.session.stream.is_ssl()) {
          if (!SSL_set_tlsext_host_name(s.session.stream.ssl().native_handle(), s.host.c_str())) {
            ec.assign(static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category());
            goto upcall;
          }
        }

        BOOST_ASIO_CORO_YIELD
        s.resolver.async_resolve(s.host, s.service,
                                 bind_handler(std::move(*this), on_resolve_t{}, _1, _2));

        if (ec) { goto upcall; }

        BOOST_ASIO_CORO_YIELD
        {
          auto& socket = s.session.stream.is_ssl() ? s.session.stream.ssl().next_layer()
                                                   : s.session.stream.plain();
          boost::asio::async_connect(socket, s.results,
                                     bind_handler(std::move(*this), on_connect_t{}, _1, _2));
        }

        if (ec) { goto upcall; }

        if (s.session.stream.is_ssl()) {
          BOOST_ASIO_CORO_YIELD
          s.session.stream.ssl().async_handshake(boost::asio::ssl::stream_base::client,
                                                 bind_handler(std::move(*this), _1, 0));

          if (ec) { goto upcall; }
        }

        {
          auto endpoint = std::move(s.endpoint);
          return this->complete_now(boost::system::error_code(), std::move(endpoint));
        }

      upcall:
        if (!is_continuation) {
          BOOST_ASIO_CORO_YIELD
          boost::asio::post(boost::beast::bind_handler(std::move(*this), ec, 0));
        }

        this->complete_now(ec, boost::asio::ip::tcp::endpoint());
      }
    }
  };

  boost::asio::async_completion<ConnectHandler,
                                void(boost::system::error_code, boost::asio::ip::tcp::endpoint)>
    init(handler);

  op(*this, std::move(host), std::move(service), std::move(init.completion_handler))({}, 0, false);

  // detail::timed_op_wrapper<
  //   boost::asio::ip::tcp::socket, detail::connect_op,
  //   typename boost::asio::async_completion<
  //     ConnectHandler,
  //     void(boost::system::error_code, boost::asio::ip::tcp::endpoint)>::completion_handler_type,
  //   void(boost::system::error_code, boost::asio::ip::tcp::endpoint)>(
  //   *this, std::move(init.completion_handler))
  //   .init(std::move(host), std::move(service));

  return init.result.get();
}

} // namespace foxy

#endif // FOXY_IMPL_CLIENT_SESSION_ASYNC_CONNECT_IMPL_HPP_
