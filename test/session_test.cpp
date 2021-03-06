//
// Copyright (c) 2018-2019 Christian Mazakas (christian dot mazakas at gmail dot
// com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/LeonineKing1199/foxy
//

#include <foxy/session.hpp>
#include <foxy/type_traits.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>

#include <boost/beast/core/ostream.hpp>

#include <boost/beast/http.hpp>
#include <boost/beast/experimental/test/stream.hpp>
#include <boost/beast/experimental/test/fail_count.hpp>

#include <iostream>

#include <catch2/catch.hpp>

namespace asio = boost::asio;
namespace http = boost::beast::http;

static_assert(
  foxy::detail::is_closable_stream_throw<boost::beast::test::stream>::value,
  "Incorrect implementation of foxy::detail::is_closable_stream_throw");

static_assert(
  foxy::detail::is_closable_stream_throw<asio::ip::tcp::socket>::value,
  "Incorrect implementation of foxy::detail::is_closable_stream_throw");

static_assert(
  foxy::detail::is_closable_stream_nothrow<asio::ip::tcp::socket>::value,
  "Incorrect implementation of foxy::detail::is_closable_stream_throw");

TEST_CASE("Our basic_session class...")
{
  SECTION("should be able to read a header")
  {
    asio::io_context io;

    auto req = http::request<http::empty_body>(http::verb::get, "/", 11);
    req.set(http::field::host, "www.google.com");

    auto test_stream = boost::beast::test::stream(io);

    boost::beast::ostream(test_stream.buffer()) << req;

    auto valid_parse  = false;
    auto valid_verb   = false;
    auto valid_target = false;

    asio::spawn([&](asio::yield_context yield) mutable {
      auto session =
        foxy::basic_session<boost::beast::test::stream>(std::move(test_stream));

      http::request_parser<http::empty_body> parser;

      session.async_read_header(parser, yield);

      auto const is_header_done = parser.is_header_done();
      auto const is_done        = parser.is_done();

      auto msg = parser.release();

      valid_parse  = is_header_done && is_done;
      valid_verb   = msg.method() == http::verb::get;
      valid_target = msg.target() == "/";
    });

    io.run();
    CHECK(valid_parse);
    CHECK(valid_verb);
    CHECK(valid_target);
  }

  SECTION("should be able to read a complete message with a body")
  {
    asio::io_context io;

    auto req = http::request<http::string_body>(http::verb::get, "/", 11);
    req.set(http::field::host, "www.google.com");

    req.body() = "I bestow the heads of virgins and the first-born sons!!!";
    req.prepare_payload();

    auto test_stream = boost::beast::test::stream(io);

    boost::beast::ostream(test_stream.buffer()) << req;

    auto valid_parse = false;
    auto valid_body  = false;

    asio::spawn([&](asio::yield_context yield) mutable {
      auto session =
        foxy::basic_session<boost::beast::test::stream>(std::move(test_stream));

      http::request_parser<http::string_body> parser;

      session.async_read(parser, yield);

      auto const is_header_done = parser.is_header_done();
      auto const is_done        = parser.is_done();

      auto msg = parser.release();

      valid_parse = is_header_done && is_done;
      valid_body  = msg.body().size() > 0;
    });

    io.run();
    CHECK(valid_parse);
    CHECK(valid_body);
  }

  SECTION("should be able to write a response")
  {
    asio::io_context io;

    auto test_stream = boost::beast::test::stream(io);
    auto peer_stream = boost::beast::test::stream(io);
    test_stream.connect(peer_stream);

    auto valid_serialization = false;

    asio::spawn([&](asio::yield_context yield) mutable {
      auto ec = boost::system::error_code();

      auto session =
        foxy::basic_session<boost::beast::test::stream>(std::move(test_stream));

      auto res = http::response<http::empty_body>(http::status::ok, 11);
      http::response_serializer<http::empty_body> serializer(res);

      session.async_write_header(serializer, yield[ec]);
      if (ec) {
        std::cout << ec.message() << "\n";
        return;
      }

      auto const is_serialization_done = peer_stream.buffer().size() > 0;
      auto const is_header_done        = serializer.is_header_done();

      session.async_write(serializer, yield[ec]);
      if (ec) {
        std::cout << ec.message() << "\n";
        return;
      }

      auto const is_done = serializer.is_done();
      auto const valid_output =
        (peer_stream.str() == "HTTP/1.1 200 OK\r\n\r\n");

      valid_serialization =
        is_serialization_done && is_header_done && is_done && valid_output;
    });

    io.run();
    CHECK(valid_serialization);
  }
}
