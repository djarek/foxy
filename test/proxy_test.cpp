//
// Copyright (c) 2018-2019 Christian Mazakas (christian dot mazakas at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/LeonineKing1199/foxy
//

#include <foxy/proxy.hpp>
#include <foxy/client_session.hpp>

#include <boost/asio/spawn.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/address_v4.hpp>

#include <boost/beast/http.hpp>

#include <memory>
#include <iostream>

#include <catch2/catch.hpp>

using boost::asio::ip::tcp;
namespace ip   = boost::asio::ip;
namespace asio = boost::asio;
namespace http = boost::beast::http;
namespace ssl  = boost::asio::ssl;

using namespace std::chrono_literals;

TEST_CASE("Our forward proxy")
{
  SECTION("should reject ill-formed requests")
  {
    asio::io_context io;

    auto was_valid_response = false;

    asio::spawn([&](asio::yield_context yield) {
      auto const addr     = ip::make_address_v4("127.0.0.1");
      auto const port     = static_cast<unsigned short>(1337);
      auto const endpoint = tcp::endpoint(addr, port);

      auto const reuse_addr = true;

      // our initial test of the relay operation will be determining if we get back a valid response
      // from a well-known website
      //
      auto const request =
        http::request<http::empty_body>(http::verb::get, "lol-some-garbage-target", 11);

      // create our proxy server and begin listening
      //
      auto proxy = std::make_shared<foxy::proxy>(io, endpoint, reuse_addr);
      proxy->async_accept();

      // create a client that connects to it locally and feeds it requests
      //
      auto client = foxy::client_session(io);
      client.async_connect("127.0.0.1", "1337", yield);

      http::response_parser<http::string_body> parser;
      client.async_request(request, parser, yield);

      auto ec = boost::system::error_code();
      client.stream.plain().shutdown(tcp::socket::shutdown_send, ec);
      client.stream.plain().close(ec);

      auto response = parser.release();

      auto const was_valid_result = response.result() == http::status::bad_request;
      auto const was_valid_body =
        response.body() ==
        "Malformed client request. Use either CONNECT <authority-uri> or <verb> <absolute-uri>";

      was_valid_response = was_valid_result && was_valid_body;
      proxy->cancel(ec);
      proxy.reset();
    });

    io.run();
    REQUIRE(was_valid_response);
  }

  SECTION("should act as a relay for absolute URIs")
  {
    asio::io_context io;

    auto was_valid_response = false;

    asio::spawn([&](asio::yield_context yield) {
      auto const addr     = ip::make_address_v4("127.0.0.1");
      auto const port     = static_cast<unsigned short>(1337);
      auto const endpoint = tcp::endpoint(addr, port);

      auto const reuse_addr = true;

      // our initial test of the relay operation will be determining if we get back a valid response
      // from a well-known website
      //
      auto const request =
        http::request<http::empty_body>(http::verb::get, "http://www.google.com:80/", 11);

      // create our proxy server and begin listening
      //
      auto proxy = std::make_shared<foxy::proxy>(io, endpoint, reuse_addr);
      proxy->async_accept();

      // create a client that connects to it locally and feeds it requests
      //
      auto client = foxy::client_session(io);
      client.async_connect("127.0.0.1", "1337", yield);

      http::response_parser<http::string_body> parser;
      client.async_request(request, parser, yield);

      auto ec = boost::system::error_code();
      client.stream.plain().shutdown(tcp::socket::shutdown_send, ec);
      client.stream.plain().close(ec);

      auto response = parser.release();

      auto const was_valid_result = response.result() == http::status::ok;
      auto const was_valid_body   = response.body().size() > 0;

      // std::cout << response.body() << "\n\n";

      was_valid_response = was_valid_result && was_valid_body;
      proxy->cancel(ec);
      proxy.reset();
    });

    io.run();
    REQUIRE(was_valid_response);
  }

  SECTION("should relay via CONNECT")
  {
    asio::io_context io;

    auto was_valid_response = false;

    asio::spawn([&](asio::yield_context yield) {
      auto const addr     = ip::make_address_v4("127.0.0.1");
      auto const port     = static_cast<unsigned short>(1337);
      auto const endpoint = tcp::endpoint(addr, port);

      auto const reuse_addr = true;

      auto proxy = std::make_shared<foxy::proxy>(io, endpoint, reuse_addr);
      proxy->async_accept();

      auto client = foxy::client_session(io);
      client.async_connect("127.0.0.1", "1337", yield);

      auto const request =
        http::request<http::empty_body>(http::verb::connect, "www.google.com:80", 11);

      http::response_parser<http::empty_body> tunnel_parser;
      tunnel_parser.skip(true);

      auto const google_request = http::request<http::empty_body>(http::verb::get, "/", 11);

      http::response_parser<http::string_body> google_parser;

      client.async_request(request, tunnel_parser, yield);
      client.async_request(google_request, google_parser, yield);

      auto ec = boost::system::error_code();
      client.stream.plain().shutdown(tcp::socket::shutdown_send, ec);
      client.stream.plain().close(ec);

      auto response = google_parser.release();

      auto const was_valid_result = response.result() == http::status::ok;
      auto const was_valid_body   = response.body().size() > 0;

      std::cout << response.body() << "\n\n";

      was_valid_response = was_valid_result && was_valid_body;
      proxy->cancel(ec);
      proxy.reset();
    });

    io.run();
    REQUIRE(was_valid_response);
  }

  // SECTION("should reject all non-persistent connections")
  // {
  //   asio::io_context io;

  //   auto was_valid_response = false;

  //   asio::spawn([&](asio::yield_context yield) {
  //     auto const src_addr     = ip::make_address_v4("127.0.0.1");
  //     auto const src_port     = static_cast<unsigned short>(1337);
  //     auto const src_endpoint = tcp::endpoint(src_addr, src_port);

  //     auto const reuse_addr = true;

  //     auto proxy = std::make_shared<foxy::proxy>(io, src_endpoint, reuse_addr);
  //     proxy->async_accept();

  //     auto client = foxy::client_session(io);
  //     client.async_connect("127.0.0.1", "1337", yield);

  //     auto request = http::request<http::empty_body>(http::verb::get, "/", 11);
  //     request.keep_alive(false);

  //     http::response_parser<http::string_body> res_parser;

  //     client.async_request(request, res_parser, yield);

  //     auto ec = boost::system::error_code();
  //     client.stream.plain().shutdown(tcp::socket::shutdown_send, ec);
  //     client.stream.plain().close(ec);

  //     auto response = res_parser.release();

  //     auto const was_valid_result = response.result() == http::status::bad_request;
  //     auto const was_valid_body =
  //       response.body() == "Connection must be persistent to allow proper tunneling\n\n";

  //     was_valid_response = was_valid_result && was_valid_body;
  //     proxy->cancel(ec);
  //     proxy.reset();
  //   });

  //   io.run();
  //   REQUIRE(was_valid_response);
  // }

  // SECTION(
  //   "should reject all unexpected bodies, requests that aren't CONNECTs and "
  //   "invalid remote targets")
  // {
  //   asio::io_context io;

  //   auto rejected_unexpected_body = false;
  //   auto rejected_non_connect     = false;
  //   auto handled_bad_remote       = false;

  //   asio::spawn([&](asio::yield_context yield) mutable {
  //     auto const src_endpoint = tcp::endpoint(ip::make_address_v4("127.0.0.1"), 1337);
  //     auto const reuse_addr   = true;

  //     auto proxy = std::make_shared<foxy::proxy>(io, src_endpoint, reuse_addr);
  //     proxy->async_accept();

  //     auto client         = foxy::client_session(io);
  //     client.opts.timeout = 5s;

  //     client.async_connect("127.0.0.1", "1337", yield);

  //     // unexpected body
  //     //
  //     {
  //       auto request = http::request<http::string_body>(http::verb::get, "/", 11);
  //       request.keep_alive(true);
  //       request.body() = "ha ha ha, this is an non-zero length body";
  //       request.prepare_payload();

  //       http::response_parser<http::string_body> res_parser;

  //       client.async_request(request, res_parser, yield);

  //       auto response = res_parser.release();

  //       auto const was_valid_result = response.result() == http::status::bad_request;
  //       auto const was_valid_body   = response.body() ==
  //                                   "Messages with bodies are not supported "
  //                                   "for establishing a tunnel\n\n";

  //       rejected_unexpected_body = was_valid_result && was_valid_body;
  //     }

  //     // non-CONNECT test
  //     //
  //     {
  //       auto request = http::request<http::empty_body>(http::verb::get, "/", 11);
  //       request.keep_alive(true);
  //       request.prepare_payload();

  //       http::response_parser<http::string_body> res_parser;

  //       client.async_request(request, res_parser, yield);

  //       auto response = res_parser.release();

  //       auto const was_valid_result = response.result() == http::status::method_not_allowed;
  //       auto const was_valid_body =
  //         response.body() == "Invalid request method. Only CONNECT is supported\n\n";
  //       rejected_non_connect = was_valid_result && was_valid_body;
  //     }

  //     // invalid remote origin
  //     //
  //     {
  //       auto request =
  //         http::request<http::empty_body>(http::verb::connect, "www.google.com:1337", 11);
  //       request.keep_alive(true);
  //       request.prepare_payload();

  //       http::response_parser<http::string_body> res_parser;

  //       client.async_request(request, res_parser, yield);

  //       auto response = res_parser.release();

  //       auto const was_valid_result = response.result() == http::status::bad_request;
  //       auto const was_valid_body   = response.body().size() > 0;

  //       handled_bad_remote = was_valid_result && was_valid_body;
  //     }

  //     auto ec = boost::system::error_code();
  //     client.stream.plain().shutdown(tcp::socket::shutdown_send, ec);
  //     client.stream.plain().close(ec);

  //     proxy->cancel(ec);
  //   });

  //   io.run();

  //   CHECK(rejected_unexpected_body);
  //   CHECK(rejected_non_connect);
  //   CHECK(handled_bad_remote);
  // }

  // SECTION("should forward a message")
  // {
  //   asio::io_context io;

  //   auto was_valid_response = false;

  //   asio::spawn([&](asio::yield_context yield) {
  //     auto const src_addr     = ip::make_address_v4("127.0.0.1");
  //     auto const src_port     = static_cast<unsigned short>(1337);
  //     auto const src_endpoint = tcp::endpoint(src_addr, src_port);

  //     auto const reuse_addr = true;

  //     auto proxy = std::make_shared<foxy::proxy>(io, src_endpoint, reuse_addr);
  //     proxy->async_accept();
  //     auto client = foxy::client_session(io);
  //     client.async_connect("127.0.0.1", "1337", yield);

  //     auto request = http::request<http::empty_body>(http::verb::connect, "www.google.com:80",
  //     11); request.prepare_payload();

  //     http::response_parser<http::string_body> res_parser;
  //     res_parser.skip(true);

  //     auto request2 = http::request<http::empty_body>(http::verb::get, "/", 11);
  //     request2.prepare_payload();

  //     http::response_parser<http::string_body> res_parser2;

  //     client.async_request(request, res_parser, yield);

  //     client.async_request(request2, res_parser2, yield);

  //     auto response = res_parser2.release();

  //     auto const was_valid_result = (response.result() == http::status::ok);
  //     auto const was_valid_body   = (response.body().size() > 0);

  //     auto ec = boost::system::error_code();
  //     client.stream.plain().shutdown(tcp::socket::shutdown_send, ec);
  //     client.stream.plain().close(ec);

  //     was_valid_response = was_valid_result && was_valid_body;
  //     proxy->cancel(ec);
  //   });

  //   io.run();
  //   REQUIRE(was_valid_response);
  // }

  // SECTION("should forward a message over an encrypted connection")
  // {
  //   asio::io_context io;

  //   auto was_valid_response = false;

  //   asio::spawn([&](asio::yield_context yield) {
  //     auto const src_addr     = ip::make_address_v4("127.0.0.1");
  //     auto const src_port     = static_cast<unsigned short>(1337);
  //     auto const src_endpoint = tcp::endpoint(src_addr, src_port);

  //     auto const reuse_addr = true;

  //     auto ctx  = ssl::context(ssl::context::method::tlsv12_client);
  //     auto opts = foxy::session_opts{ctx, 5s};

  //     auto proxy = std::make_shared<foxy::proxy>(io, src_endpoint, reuse_addr, opts);
  //     proxy->async_accept();

  //     auto client         = foxy::client_session(io);
  //     client.opts.timeout = 5s;

  //     client.async_connect("127.0.0.1", "1337", yield);

  //     auto request = http::request<http::empty_body>(http::verb::connect, "www.google.com:443",
  //     11); request.prepare_payload();

  //     http::response_parser<http::string_body> res_parser;
  //     res_parser.skip(true);

  //     auto request2 = http::request<http::empty_body>(http::verb::get, "/", 11);
  //     request2.prepare_payload();

  //     http::response_parser<http::string_body> res_parser2;

  //     client.async_request(request, res_parser, yield);

  //     client.async_request(request2, res_parser2, yield);

  //     auto response = res_parser2.release();

  //     auto const was_valid_result = (response.result() == http::status::ok);
  //     auto const was_valid_body   = (response.body().size() > 0);

  //     auto ec = boost::system::error_code();
  //     client.stream.plain().shutdown(tcp::socket::shutdown_send, ec);
  //     client.stream.plain().close(ec);

  //     was_valid_response = was_valid_result && was_valid_body;
  //     proxy->cancel(ec);
  //   });

  //   io.run();
  //   REQUIRE(was_valid_response);
  // }
}
