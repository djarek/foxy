#ifndef FOXY_SESSION_HPP
#define FOXY_SESSION_HPP

#include "foxy/multi_stream.hpp"

#include <boost/asio/async_result.hpp>
#include <boost/asio/steady_timer.hpp>

#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/core/flat_buffer.hpp>

namespace foxy
{

struct session
{
public:
  using stream_type     = ::foxy::multi_stream;
  using buffer_type     = boost::beast::flat_buffer;
  using timer_type      = boost::asio::steady_timer;
  using ssl_stream_type = boost::beast::ssl_stream<stream_type&>;

  stream_type stream;
  buffer_type buffer;
  timer_type  timer;

  session()               = delete;
  session(session const&) = delete;
  session(session&&)      = default;

  explicit session(boost::asio::io_context& io);
  explicit session(stream_type stream_);

  using executor_type = decltype(stream.get_executor());

  auto get_executor() -> executor_type;

  template <class Parser, class ReadHandler>
  auto
  async_read_header(
    Parser&       parser,
    ReadHandler&& handler
  ) & -> BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void(boost::system::error_code, std::size_t));

  template <class Parser, class ReadHandler>
  auto
  async_read(
    Parser&       parser,
    ReadHandler&& handler
  ) & -> BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void(boost::system::error_code, std::size_t));

  template <class Serializer, class WriteHandler>
  auto
  async_write_header(
    Serializer&    serializer,
    WriteHandler&& handler
  ) & -> BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void(boost::system::error_code, std::size_t));

  template <class Serializer, class WriteHandler>
  auto
  async_write(
    Serializer&    serializer,
    WriteHandler&& handler
  ) & -> BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void(boost::system::error_code, std::size_t));
};

} // foxy

#include "foxy/impl/session.impl.hpp"

#endif // FOXY_SESSION_HPP