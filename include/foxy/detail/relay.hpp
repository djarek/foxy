//
// Copyright (c) 2018-2018 Christian Mazakas (christian dot mazakas at gmail dot
// com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/LeonineKing1199/foxy
//

#ifndef FOXY_DETAIL_RELAY_HPP_
#define FOXY_DETAIL_RELAY_HPP_

#include <foxy/session.hpp>

#include <boost/optional/optional.hpp>

#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/http/buffer_body.hpp>
#include <boost/beast/http/error.hpp>

#include <array>
#include <iostream>

namespace foxy
{
namespace detail
{
template <class Stream, class RelayHandler>
struct relay_op : boost::asio::coroutine
{
private:
  struct state
  {
    std::array<char, 2048> buffer;

    ::foxy::basic_session<Stream>& server;
    ::foxy::basic_session<Stream>& client;

    boost::optional<
      boost::beast::http::request_parser<boost::beast::http::buffer_body>>
      req_parser;

    boost::optional<
      boost::beast::http::request_serializer<boost::beast::http::buffer_body>>
      req_sr;

    boost::optional<
      boost::beast::http::response_parser<boost::beast::http::buffer_body>>
      res_parser;

    boost::optional<
      boost::beast::http::response_serializer<boost::beast::http::buffer_body>>
      res_sr;

    boost::asio::executor_work_guard<decltype(server.get_executor())> work;

    explicit state(RelayHandler const&            handler,
                   ::foxy::basic_session<Stream>& server_,
                   ::foxy::basic_session<Stream>& client_)
      : server(server_)
      , client(client_)
      , work(server.get_executor())
    {
    }
  };

  boost::beast::handler_ptr<state, RelayHandler> p_;

public:
  relay_op()                = delete;
  relay_op(relay_op const&) = default;
  relay_op(relay_op&&)      = default;

  template <class DeducedHandler>
  relay_op(::foxy::basic_session<Stream>& server,
           ::foxy::basic_session<Stream>& client,
           DeducedHandler&&               handler)
    : p_(std::forward<DeducedHandler>(handler), server, client)
  {
  }

  using executor_type = boost::asio::associated_executor_t<
    RelayHandler,
    decltype((std::declval<::foxy::basic_session<Stream>&>().get_executor()))>;

  using allocator_type = boost::asio::associated_allocator_t<RelayHandler>;

  auto get_executor() const noexcept -> executor_type
  {
    return boost::asio::get_associated_executor(p_.handler(),
                                                p_->server.get_executor());
  }

  auto get_allocator() const noexcept -> allocator_type
  {
    return boost::asio::get_associated_allocator(p_.handler());
  }

  auto operator()(boost::system::error_code ec,
                  std::size_t const         bytes_transferred,
                  bool const                is_continuation = true) -> void;
};

template <class Stream, class RelayHandler>
auto relay_op<Stream, RelayHandler>::
     operator()(boost::system::error_code ec,
           std::size_t const         bytes_transferred,
           bool const                is_continuation) -> void
{
  using namespace std::placeholders;
  using boost::beast::bind_handler;

  namespace http = boost::beast::http;

  auto& s = *p_;
  BOOST_ASIO_CORO_REENTER(*this)
  {
    s.req_parser = boost::none;
    s.req_sr     = boost::none;
    s.req_parser = boost::none;
    s.res_sr     = boost::none;

    s.req_parser.emplace();
    s.req_sr.emplace(s.req_parser->get());

    s.res_parser.emplace();
    s.res_sr.emplace(s.res_parser->get());

    BOOST_ASIO_CORO_YIELD
    s.server.async_read_header(*s.req_parser, std::move(*this));
    if (ec) { goto upcall; }

    BOOST_ASIO_CORO_YIELD
    s.client.async_write_header(*s.req_sr, std::move(*this));
    if (ec) { goto upcall; }

    do {
      if (!s.req_parser->is_done()) {
        s.req_parser->get().body().data = s.buffer.data();
        s.req_parser->get().body().size = s.buffer.size();

        BOOST_ASIO_CORO_YIELD
        s.server.async_read(*s.req_parser, std::move(*this));
        if (ec == http::error::need_buffer) { ec = {}; }
        if (ec) { goto upcall; }

        s.req_parser->get().body().size =
          s.buffer.size() - s.req_parser->get().body().size;
        s.req_parser->get().body().data = s.buffer.data();
        s.req_parser->get().body().more = !s.req_parser->is_done();

      } else {
        s.req_parser->get().body().data = nullptr;
        s.req_parser->get().body().size = 0;
      }

      BOOST_ASIO_CORO_YIELD
      s.client.async_write(*s.req_sr, std::move(*this));
      if (ec == http::error::need_buffer) { ec = {}; }
      if (ec) { goto upcall; }
    } while (!s.req_parser->is_done() && !s.req_sr->is_done());

    BOOST_ASIO_CORO_YIELD
    s.client.async_read_header(*s.res_parser, std::move(*this));
    if (ec) { goto upcall; }

    BOOST_ASIO_CORO_YIELD
    s.server.async_write_header(*s.res_sr, std::move(*this));
    if (ec) { goto upcall; }

    do {
      if (!s.res_parser->is_done()) {
        s.res_parser->get().body().data = s.buffer.data();
        s.res_parser->get().body().size = s.buffer.size();

        BOOST_ASIO_CORO_YIELD
        s.client.async_read(*s.res_parser, std::move(*this));
        if (ec == http::error::need_buffer) { ec = {}; }
        if (ec) { goto upcall; }

        s.res_parser->get().body().size =
          s.buffer.size() - s.res_parser->get().body().size;
        s.res_parser->get().body().data = s.buffer.data();
        s.res_parser->get().body().more = !s.res_parser->is_done();

      } else {
        s.res_parser->get().body().data = nullptr;
        s.res_parser->get().body().size = 0;
      }

      BOOST_ASIO_CORO_YIELD
      s.server.async_write(*s.res_sr, std::move(*this));
      if (ec == http::error::need_buffer) { ec = {}; }
      if (ec) { goto upcall; }
    } while (!s.res_parser->is_done() && !s.res_sr->is_done());

    {
      auto work = std::move(s.work);
      return p_.invoke(boost::system::error_code(), bytes_transferred);
    }

  upcall:
    if (!is_continuation) {
      BOOST_ASIO_CORO_YIELD
      boost::asio::post(bind_handler(std::move(*this), ec, 0));
    }
    auto work = std::move(s.work);
    p_.invoke(ec, 0);
  }
}

template <class Stream, class RelayHandler>
auto async_relay(::foxy::basic_session<Stream>& server,
                 ::foxy::basic_session<Stream>& client,
                 RelayHandler&&                 handler)
  -> BOOST_ASIO_INITFN_RESULT_TYPE(RelayHandler,
                                   void(boost::system::error_code, std::size_t))
{
  boost::asio::async_completion<RelayHandler,
                                void(boost::system::error_code, std::size_t)>
    init(handler);

  relay_op<Stream,
           BOOST_ASIO_HANDLER_TYPE(
             RelayHandler, void(boost::system::error_code, std::size_t))>(
    server, client, std::move(init.completion_handler))({}, 0, false);

  return init.result.get();
}

} // namespace detail
} // namespace foxy

#endif // FOXY_DETAIL_RELAY_HPP_