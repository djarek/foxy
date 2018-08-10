#ifndef FOXY_CLIENT_SESSION_HPP_
#define FOXY_CLIENT_SESSION_HPP_

#include "foxy/session.hpp"

#include <boost/system/error_code.hpp>

#include <boost/asio/async_result.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/associated_allocator.hpp>
#include <boost/asio/connect.hpp>

#include <boost/beast/core/handler_ptr.hpp>
#include <boost/beast/core/bind_handler.hpp>

#include <string>
#include <utility>
#include <functional>

namespace foxy
{
struct client_session : public session
{
public:
  client_session()                      = delete;
  client_session(client_session const&) = default;
  client_session(client_session&&)      = default;

  explicit client_session(boost::asio::io_context& io);

  template <class ConnectHandler>
  auto
  async_connect(
    std::string      host,
    std::string      service,
    ConnectHandler&& handler
  ) & -> BOOST_ASIO_INITFN_RESULT_TYPE(
    ConnectHandler, void(boost::system::error_code, boost::asio::ip::tcp::endpoint));
};

} // foxy

#include "foxy/impl/client_session.impl.hpp"

#endif // FOXY_CLIENT_SESSION_HPP_