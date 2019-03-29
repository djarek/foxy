//
// Copyright (c) 2018-2019 Christian Mazakas (christian dot mazakas at gmail dot
// com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/LeonineKing1199/foxy
//

#ifndef FOXY_SESSION_IMPL_HPP_
#define FOXY_SESSION_IMPL_HPP_

#include <foxy/session.hpp>

namespace foxy
{
template <class Stream, class X>
foxy::basic_session<Stream, X>::basic_session(boost::asio::io_context& io, session_opts opts_)
  : opts(std::move(opts_))
  , stream(opts.ssl_ctx ? stream_type(io, *opts.ssl_ctx) : stream_type(io))
{
}

template <class Stream, class X>
foxy::basic_session<Stream, X>::basic_session(stream_type stream_, session_opts opts_)
  : opts(std::move(opts_))
  , stream(std::move(stream_))
{
}

template <class Stream, class X>
auto
foxy::basic_session<Stream, X>::get_executor() -> executor_type
{
  return stream.get_executor();
}

template <class Stream, class X>
template <class Parser, class ReadHandler>
auto
foxy::basic_session<Stream, X>::async_read(Parser& parser, ReadHandler&& handler) & ->
  typename boost::asio::async_result<std::decay_t<ReadHandler>,
                                     void(boost::system::error_code, std::size_t)>::return_type
{
  return boost::beast::http::async_read(stream, buffer, parser, std::forward<ReadHandler>(handler));
}

template <class Stream, class X>
template <class Parser, class ReadHandler>
auto
foxy::basic_session<Stream, X>::async_read_header(Parser& parser, ReadHandler&& handler) & ->
  typename boost::asio::async_result<std::decay_t<ReadHandler>,
                                     void(boost::system::error_code, std::size_t)>::return_type
{
  return boost::beast::http::async_read_header(stream, buffer, parser,
                                               std::forward<ReadHandler>(handler));
}

template <class Stream, class X>
template <class Serializer, class WriteHandler>
auto
foxy::basic_session<Stream, X>::async_write_header(Serializer&    serializer,
                                                   WriteHandler&& handler) & ->
  typename boost::asio::async_result<std::decay_t<WriteHandler>,
                                     void(boost::system::error_code, std::size_t)>::return_type
{
  return boost::beast::http::async_write_header(stream, serializer,
                                                std::forward<WriteHandler>(handler));
}

template <class Stream, class X>
template <class Serializer, class WriteHandler>
auto
foxy::basic_session<Stream, X>::async_write(Serializer& serializer, WriteHandler&& handler) & ->
  typename boost::asio::async_result<std::decay_t<WriteHandler>,
                                     void(boost::system::error_code, std::size_t)>::return_type
{
  return boost::beast::http::async_write(stream, serializer, std::forward<WriteHandler>(handler));
}

} // namespace foxy

#endif // FOXY_SESSION_IMPL_HPP_
