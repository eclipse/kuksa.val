/*
 * ******************************************************************************
 * Copyright (c) 2019 Robert Bosch GmbH.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/org/documents/epl-2.0/index.php
 *
 *  Contributors:
 *      Robert Bosch GmbH - initial API and functionality
 * *****************************************************************************
 */

#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/make_unique.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>
#include <utility>
#include <stdexcept>

#include "detect_ssl.hpp"
#include "ssl_stream.hpp"

#include "WebSockHttpFlexServer.hpp"

#include "IRestHandler.hpp"
#include "IVssCommandProcessor.hpp"
#include "WsChannel.hpp"
#include "ILogger.hpp"

using RequestHandler = std::function<std::string(const std::string &, WsChannel &)>;
using Listeners = std::vector<std::pair<ObserverType,std::shared_ptr<IVssCommandProcessor>>>;
using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;               // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http;            // from <boost/beast/http.hpp>
namespace websocket = boost::beast::websocket;  // from <boost/beast/websocket.hpp>

namespace {
  // forward declaration for classes that are defined below
  class PlainWebsocketSession;
  class SslWebsocketSession;
  class PlainHttpSession;
  class SslHttpSession;
  class BeastListener;

/**
 * @class ConnectionHandler
 * @brief Helper class to handle connection between \ref WsChannel and actual sessions
 */
  class ConnectionHandler {
      // Allow access to connection information from this file implementation details
      friend class ::WebSockHttpFlexServer;

    private:
      std::mutex mPlainWebSock_;
      std::mutex mSslWebSock_;
      std::mutex mPlainHttp_;
      std::mutex mSslHttp_;

      std::unordered_map<const PlainWebsocketSession *,
                         std::shared_ptr<WsChannel>> connPlainWebSock_;
      std::unordered_map<const SslWebsocketSession *,
                         std::shared_ptr<WsChannel>> connSslWebSock_;
      std::unordered_map<const PlainHttpSession *,
                         std::shared_ptr<WsChannel>> connPlainHttp_;
      std::unordered_map<const SslHttpSession *,
                         std::shared_ptr<WsChannel>> connSslHttp_;


    public:
      ConnectionHandler() = default;

      /**
       * @brief Add new client for plain Web-Socket session
       * @param session New session to add
       * @return \ref WsChannel with new connection information
       */
      WsChannel& AddClient(const PlainWebsocketSession * session) {
        auto newChannel = std::make_shared<WsChannel>();
        std::lock_guard<std::mutex> lock(mPlainWebSock_);

        newChannel->setConnID(reinterpret_cast<uint64_t>(session));
        newChannel->setType(WsChannel::Type::WEBSOCKET_PLAIN);
        connPlainWebSock_[session] = newChannel;

        return *newChannel;
      }

      /**
       * @brief Add new client for SSL Web-Socket session
       * @param session New session to add
       * @return \ref WsChannel with new connection information
       */
      WsChannel& AddClient(const SslWebsocketSession * session) {
        auto newChannel = std::make_shared<WsChannel>();
        std::lock_guard<std::mutex> lock(mSslWebSock_);

        newChannel->setConnID(reinterpret_cast<uint64_t>(session));
        newChannel->setType(WsChannel::Type::WEBSOCKET_SSL);
        connSslWebSock_[session] = newChannel;

        return *newChannel;
      }

      /**
       * @brief Add new client for plain HTTP session
       * @param session New session to add
       * @return \ref WsChannel with new connection information
       */
      WsChannel& AddClient(const PlainHttpSession * session) {
        auto newChannel = std::make_shared<WsChannel>();
        std::lock_guard<std::mutex> lock(mPlainHttp_);

        newChannel->setConnID(reinterpret_cast<uint64_t>(session));
        newChannel->setType(WsChannel::Type::HTTP_PLAIN);
        connPlainHttp_[session] = newChannel;

        return *newChannel;
      }

      /**
       * @brief Add new client for SSL HTTP session
       * @param session New session to add
       * @return \ref WsChannel with new connection information
       */
      WsChannel& AddClient(const SslHttpSession * session) {
        auto newChannel = std::make_shared<WsChannel>();
        std::lock_guard<std::mutex> lock(mSslHttp_);

        newChannel->setConnID(reinterpret_cast<uint64_t>(session));
        newChannel->setType(WsChannel::Type::HTTP_SSL);
        connSslHttp_[session] = newChannel;

        return *newChannel;
      }

      /**
       * @brief Remove client for plain Web-Socket session
       * @param session Existing session to remove
       */
      void RemoveClient(const PlainWebsocketSession * session) {
        std::lock_guard<std::mutex> lock(mPlainWebSock_);
        connPlainWebSock_.erase(session);
      }

      /**
       * @brief Remove client for SSL Web-Socket session
       * @param session Existing session to remove
       */
      void RemoveClient(const SslWebsocketSession * session) {
        std::lock_guard<std::mutex> lock(mSslWebSock_);
        connSslWebSock_.erase(session);
      }

      /**
       * @brief Remove client for plain HTTP session
       * @param session Existing session to remove
       */
      void RemoveClient(const PlainHttpSession * session) {
        std::lock_guard<std::mutex> lock(mPlainHttp_);
        connPlainHttp_.erase(session);
      }

      /**
       * @brief Remove client for SSL HTTP session
       * @param session Existing session to remove
       */
      void RemoveClient(const SslHttpSession * session) {
        std::lock_guard<std::mutex> lock(mSslHttp_);
        connSslHttp_.erase(session);
      }
  };

  /**** Local variables ****/


  // Boost.Beast helper state variables
  ConnectionHandler                        connHandler;
  std::shared_ptr<BeastListener>           connListener;
  std::shared_ptr<boost::asio::io_context> ioc;
  ssl::context                             ctx{ssl::context::sslv23};
  std::vector<std::thread>                 iocRunners;

  /// Are allowed plain Web-socket/HTTP connections
  bool allowInsecureConns = false;

  std::shared_ptr<ILogger> logger;
  std::shared_ptr<IRestHandler> rest2json;

  const unsigned DEFAULT_TIMEOUT_VALUE   = 60u;   // in seconds
  const unsigned WEBSOCKET_TIMEOUT_VALUE = DEFAULT_TIMEOUT_VALUE;
  const unsigned HTTP_TIMEOUT_VALUE      = DEFAULT_TIMEOUT_VALUE;


  /**** Boost.Beast implementation below ****/

  /// Report a failure
  void
  fail(boost::system::error_code ec, char const* what)
  {
    logger->Log(LogLevel::ERROR, std::string(what) + ": " + ec.message());
  }

  /// Report a failure and remove client from active connections that are tracked
  template <typename T>
  void
  fail(const T* fromType, boost::system::error_code ec, char const* what)
  {
    fail(ec, what);
    logger->Log(LogLevel::ERROR, "Connection error detected, remove client from active connections");
    connHandler.RemoveClient(fromType);
  }

#if 0
  // Return a reasonable mime type based on the extension of a file.
  boost::beast::string_view
  mimeType(boost::beast::string_view path)
  {
    using boost::beast::iequals;
    auto const ext = [&path]
                      {
      auto const pos = path.rfind(".");
      if(pos == boost::beast::string_view::npos)
        return boost::beast::string_view{};
      return path.substr(pos);
                      }();
                      if(iequals(ext, ".htm"))  return "text/html";
                      if(iequals(ext, ".html")) return "text/html";
                      if(iequals(ext, ".php"))  return "text/html";
                      if(iequals(ext, ".css"))  return "text/css";
                      if(iequals(ext, ".txt"))  return "text/plain";
                      if(iequals(ext, ".js"))   return "application/javascript";
                      if(iequals(ext, ".json")) return "application/json";
                      if(iequals(ext, ".xml"))  return "application/xml";
                      if(iequals(ext, ".swf"))  return "application/x-shockwave-flash";
                      if(iequals(ext, ".flv"))  return "video/x-flv";
                      if(iequals(ext, ".png"))  return "image/png";
                      if(iequals(ext, ".jpe"))  return "image/jpeg";
                      if(iequals(ext, ".jpeg")) return "image/jpeg";
                      if(iequals(ext, ".jpg"))  return "image/jpeg";
                      if(iequals(ext, ".gif"))  return "image/gif";
                      if(iequals(ext, ".bmp"))  return "image/bmp";
                      if(iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
                      if(iequals(ext, ".tiff")) return "image/tiff";
                      if(iequals(ext, ".tif"))  return "image/tiff";
                      if(iequals(ext, ".svg"))  return "image/svg+xml";
                      if(iequals(ext, ".svgz")) return "image/svg+xml";
                      return "application/text";
  }

  // Append an HTTP rel-path to a local filesystem path.
  // The returned path is normalized for the platform.
  std::string
  fixPath(
      boost::beast::string_view base,
      boost::beast::string_view path)
  {
    if(base.empty())
      return path.to_string();
    std::string result = base.to_string();
  #if BOOST_MSVC
    char constexpr path_separator = '\\';
    if(result.back() == path_separator)
      result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for(auto& c : result)
      if(c == '/')
        c = path_separator;
  #else
    char constexpr path_separator = '/';
    if(result.back() == path_separator)
      result.resize(result.size() - 1);
    result.append(path.data(), path.size());
  #endif
    return result;
  }

  // This function produces an HTTP response for the given
  // request. The type of the response object depends on the
  // contents of the request, so the interface requires the
  // caller to pass a generic lambda for receiving the response.
  // TODO: in future this shall be replaced with REST handler as IVssCommandProcessor listener
  template<
  class Body, class Allocator,
  class Send>
  void
  defaultHttpRequestHandler(
      boost::beast::string_view doc_root,
      http::request<Body, http::basic_fields<Allocator>>&& req,
      Send&& send)
  {
      // Returns a bad request response
      auto const bad_request =
          [&req](boost::beast::string_view why)
          {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = why.to_string();
        res.prepare_payload();
        return res;
          };

      // Returns a not found response
      auto const not_found =
          [&req](boost::beast::string_view target)
          {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + target.to_string() + "' was not found.";
        res.prepare_payload();
        return res;
          };

      // Returns a server error response
      auto const server_error =
          [&req](boost::beast::string_view what)
          {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + what.to_string() + "'";
        res.prepare_payload();
        return res;
          };

      // Make sure we can handle the method
      if( req.method() != http::verb::get &&
          req.method() != http::verb::head)
        return send(bad_request("Unknown HTTP-method"));

      // Request path must be absolute and not contain "..".
      if( req.target().empty() ||
          req.target()[0] != '/' ||
          req.target().find("..") != boost::beast::string_view::npos)
        return send(bad_request("Illegal request-target"));

      // Build the path to the requested file
      std::string path = fixPath(doc_root, req.target());
      if(req.target().back() == '/')
        path.append("index.html");

      // Attempt to open the file
      boost::beast::error_code ec;
      http::file_body::value_type body;
      body.open(path.c_str(), boost::beast::file_mode::scan, ec);

      // Handle the case where the file doesn't exist
      if(ec == boost::system::errc::no_such_file_or_directory)
        return send(not_found(req.target()));

      // Handle an unknown error
      if(ec)
        return send(server_error(ec.message()));

      // Cache the size since we need it after the move
      auto const size = body.size();

      // Respond to HEAD request
      if(req.method() == http::verb::head)
      {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mimeType(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
      }

      // Respond to GET request
      http::response<http::file_body> res{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(http::status::ok, req.version())};
      res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
      res.set(http::field::content_type, mimeType(path));
      res.content_length(size);
      res.keep_alive(req.keep_alive());
      return send(std::move(res));
  }
#endif
  //------------------------------------------------------------------------------

  //------------------------------------------------------------------------------

  // Echoes back all received WebSocket messages.
  // This uses the Curiously Recurring Template Pattern so that
  // the same code works with both SSL streams and regular sockets.
  template<class Derived>
  class WebSocketSession
  {
      // Access the derived class, this is part of
      // the Curiously Recurring Template Pattern idiom.
      Derived&
      derived()
      {
        return static_cast<Derived&>(*this);
      }

      boost::beast::multi_buffer bufferRead_;
      boost::beast::multi_buffer bufferWrite_;
      char ping_state_ = 0;

    protected:
      boost::asio::strand<
      boost::asio::io_context::executor_type> strand_;
      boost::asio::steady_timer timer_;
      RequestHandler requestHandler_;
      WsChannel channel;
    public:
      // Construct the session
      explicit
      WebSocketSession(boost::asio::io_context& ioc,
                        RequestHandler requestHandler)
      : strand_(ioc.get_executor())
      , timer_(ioc,
          (std::chrono::steady_clock::time_point::max)())
      , requestHandler_(requestHandler)
      {
      }

      // Start the asynchronous operation
      template<class Body, class Allocator>
      void
      doAccept(http::request<Body, http::basic_fields<Allocator>> req)
      {
          // Set the control callback. This will be called
          // on every incoming ping, pong, and close frame.
          derived().ws().control_callback(
              std::bind(
                  &WebSocketSession::on_control_callback,
                  this,
                  std::placeholders::_1,
                  std::placeholders::_2));

          // Set the timer
          timer_.expires_after(std::chrono::seconds(WEBSOCKET_TIMEOUT_VALUE));

          // Accept the websocket handshake
          derived().ws().async_accept(
              req,
              boost::asio::bind_executor(
                  strand_,
                  std::bind(
                      &WebSocketSession::onAccept,
                      derived().shared_from_this(),
                      std::placeholders::_1)));
      }

      void
      onAccept(boost::system::error_code ec)
      {
        // Happens when the timer closes the socket
        if(ec == boost::asio::error::operation_aborted)
          return;

        if(ec) {
          fail<>(&derived(), ec, "accept");
          return;
        }

        // Read a message
        doRead();
      }

      // Called when the timer expires.
      void
      onTimer(boost::system::error_code ec)
      {
        if(ec && ec != boost::asio::error::operation_aborted) {
          fail<>(&derived(), ec, "timer");
          return;
        }

        // See if the timer really expired since the deadline may have moved.
        if(timer_.expiry() <= std::chrono::steady_clock::now())
        {
          // If this is the first time the timer expired,
          // send a ping to see if the other end is there.
          if(derived().ws().is_open() && ping_state_ == 0)
          {
            // Note that we are sending a ping
            ping_state_ = 1;

            // Set the timer
            timer_.expires_after(std::chrono::seconds(WEBSOCKET_TIMEOUT_VALUE));

            // Now send the ping
            derived().ws().async_ping({},
                boost::asio::bind_executor(
                    strand_,
                    std::bind(
                        &WebSocketSession::onPing,
                        derived().shared_from_this(),
                        std::placeholders::_1)));
          }
          else
          {
            // The timer expired while trying to handshake,
            // or we sent a ping and it never completed or
            // we never got back a control frame, so close.

            derived().doTimeout();
            return;
          }
        }

        // Wait on the timer
        timer_.async_wait(
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &WebSocketSession::onTimer,
                    derived().shared_from_this(),
                    std::placeholders::_1)));
      }

      // Called to indicate activity from the remote peer
      void
      activity()
      {
        // Note that the connection is alive
        ping_state_ = 0;

        // Set the timer
        timer_.expires_after(std::chrono::seconds(WEBSOCKET_TIMEOUT_VALUE));
      }

      // Called after a ping is sent.
      void
      onPing(boost::system::error_code ec)
      {
        // Happens when the timer closes the socket
        if(ec == boost::asio::error::operation_aborted)
          return;

        if(ec) {
          fail<>(&derived(), ec, "ping");
          return;
        }

        // Note that the ping was sent.
        if(ping_state_ == 1)
        {
          ping_state_ = 2;
        }
        else
        {
          // ping_state_ could have been set to 0
          // if an incoming control frame was received
          // at exactly the same time we sent a ping.
          BOOST_ASSERT(ping_state_ == 0);
        }
      }

      void
      on_control_callback(
          websocket::frame_type kind,
          boost::beast::string_view payload)
      {
        boost::ignore_unused(kind, payload);

        // Note that there is activity
        activity();
      }

      void
      doRead()
      {
        // Read a message into our buffer
        derived().ws().async_read(
            bufferRead_,
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &WebSocketSession::onRead,
                    derived().shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2)));
      }

      void
      onRead(
          boost::system::error_code ec,
          std::size_t bytesTransferred)
      {
        boost::ignore_unused(bytesTransferred);

        // Happens when the timer closes the socket
        if(ec == boost::asio::error::operation_aborted)
          return;

        // This indicates that the websocket_session was closed
        if(ec == websocket::error::closed)
          return;

        if(ec)
          fail(ec, "read");

        // Note that there is activity
        activity();

        derived().ws().text(derived().ws().got_text());

        std::string response = requestHandler_(boost::beast::buffers_to_string(bufferRead_.data()), channel);
        bufferRead_.consume(bytesTransferred); // clear existing buffer data

        // send response
        write(response);

        // do another read
        doRead();
      }

      void
      write(const std::string &message) {
        // TODO: add queuing as async_write should be called one at a time
        boost::asio::buffer_copy(bufferWrite_.prepare(message.size()), boost::asio::buffer(message));
        bufferWrite_.commit(message.size()); // commit copied data for write

        // send message
        derived().ws().async_write(
            bufferWrite_.data(),
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &WebSocketSession::onWrite,
                    derived().shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2)));
      }

      void
      onWrite(
          boost::system::error_code ec,
          std::size_t bytesTransferred)
      {
        boost::ignore_unused(bytesTransferred);

        // Happens when the timer closes the socket
        if(ec == boost::asio::error::operation_aborted)
          return;

        if(ec) {
          fail<>(&derived(), ec, "write");
          return;
        }

        // Clear the buffer
        bufferWrite_.consume(bytesTransferred);

        // Do another read
        //doRead();
      }
  };

  // Handles a plain WebSocket connection
  class PlainWebsocketSession
      : public WebSocketSession<PlainWebsocketSession>
  , public std::enable_shared_from_this<PlainWebsocketSession>
  {
      websocket::stream<tcp::socket> ws_;
      bool close_ = false;

    public:
      // Create the session
      explicit
      PlainWebsocketSession(tcp::socket socket, RequestHandler requestHandler)
      : WebSocketSession<PlainWebsocketSession>(
          socket.get_executor().context(), requestHandler)
          , ws_(std::move(socket))
          {
          }

      // Called by the base class
      websocket::stream<tcp::socket>&
      ws()
      {
        return ws_;
      }

      // Start the asynchronous operation
      template<class Body, class Allocator>
      void
      run(http::request<Body, http::basic_fields<Allocator>> req)
      {
          channel = connHandler.AddClient(this);

          // Run the timer. The timer is operated
          // continuously, this simplifies the code.
          onTimer({});

          // Accept the WebSocket upgrade request
          doAccept(std::move(req));
      }

      void
      doTimeout()
      {
        // This is so the close can have a timeout
        if(close_)
          return;
        close_ = true;

        // Set the timer
        timer_.expires_after(std::chrono::seconds(WEBSOCKET_TIMEOUT_VALUE));

        // Close the WebSocket Connection
        ws_.async_close(
            websocket::close_code::normal,
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &PlainWebsocketSession::on_close,
                    shared_from_this(),
                    std::placeholders::_1)));
      }

      void
      on_close(boost::system::error_code ec)
      {
        // Happens when close times out
        if(ec == boost::asio::error::operation_aborted)
          return;

        if(ec) {
          fail<>(this, ec, "close");
          return;
        }

        // At this point the connection is gracefully closed
      }
  };

  // Handles an SSL WebSocket connection
  class SslWebsocketSession
      : public WebSocketSession<SslWebsocketSession>
  , public std::enable_shared_from_this<SslWebsocketSession>
  {
      websocket::stream<ssl_stream<tcp::socket>> ws_;
      boost::asio::strand<
      boost::asio::io_context::executor_type> strand_;
      bool eof_ = false;

    public:
      // Create the http_session
      explicit
      SslWebsocketSession(ssl_stream<tcp::socket> stream, RequestHandler requestHandler)
      : WebSocketSession<SslWebsocketSession>(
          stream.get_executor().context(), requestHandler)
          , ws_(std::move(stream))
          , strand_(ws_.get_executor())
          {
          }

      // Called by the base class
      websocket::stream<ssl_stream<tcp::socket>>&
      ws()
      {
        return ws_;
      }

      // Start the asynchronous operation
      template<class Body, class Allocator>
      void
      run(http::request<Body, http::basic_fields<Allocator>> req)
      {
          channel = connHandler.AddClient(this);

          // Run the timer. The timer is operated
          // continuously, this simplifies the code.
          onTimer({});

          // Accept the WebSocket upgrade request
          doAccept(std::move(req));
      }

      void
      doEof()
      {
        eof_ = true;

        // Set the timer
        timer_.expires_after(std::chrono::seconds(WEBSOCKET_TIMEOUT_VALUE));

        // Perform the SSL shutdown
        ws_.next_layer().async_shutdown(
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &SslWebsocketSession::onShutdown,
                    shared_from_this(),
                    std::placeholders::_1)));
      }

      void
      onShutdown(boost::system::error_code ec)
      {
        // Happens when the shutdown times out
        if(ec == boost::asio::error::operation_aborted)
          return;

        if(ec) {
          fail<>(this, ec, "shutdown");
          return;
        }

        connHandler.RemoveClient(this);
        // At this point the connection is closed gracefully
      }

      void
      doTimeout()
      {
        // If this is true it means we timed out performing the shutdown
        if(eof_)
          return;

        // Start the timer again
        timer_.expires_at(
            (std::chrono::steady_clock::time_point::max)());
        onTimer({});
        doEof();
      }
  };

  template<class Body, class Allocator>
  void
  makeWebsocketSession(
      tcp::socket socket,
      http::request<Body, http::basic_fields<Allocator>> req,
      RequestHandler requestHandler)
  {
      std::make_shared<PlainWebsocketSession>(
          std::move(socket), requestHandler)->run(std::move(req));
  }

  template<class Body, class Allocator>
  void
  makeWebsocketSession(
      ssl_stream<tcp::socket> stream,
      http::request<Body, http::basic_fields<Allocator>> req,
      RequestHandler requestHandler)
  {
      std::make_shared<SslWebsocketSession>(
          std::move(stream), requestHandler)->run(std::move(req));
  }

  //------------------------------------------------------------------------------

  // Handles an HTTP server connection.
  // This uses the Curiously Recurring Template Pattern so that
  // the same code works with both SSL streams and regular sockets.
  template<class Derived>
  class HttpSession
  {
      // Access the derived class, this is part of
      // the Curiously Recurring Template Pattern idiom.
      Derived&
      derived()
      {
        return static_cast<Derived&>(*this);
      }

      // This queue is used for HTTP pipelining.
      class queue
      {
          enum
          {
            // Maximum number of responses we will queue
            limit = 8
          };

          // The type-erased, saved work item
          struct work
          {
              virtual ~work() = default;
              virtual void operator()() = 0;
          };

          HttpSession& self_;
          std::vector<std::unique_ptr<work>> items_;

        public:
          explicit
          queue(HttpSession& self)
          : self_(self)
          {
            static_assert(limit > 0, "queue limit must be positive");
            items_.reserve(limit);
          }

          // Returns `true` if we have reached the queue limit
          bool
          is_full() const
          {
            return items_.size() >= limit;
          }

          // Called when a message finishes sending
          // Returns `true` if the caller should initiate a read
          bool
          onWrite()
          {
            BOOST_ASSERT(! items_.empty());
            auto const was_full = is_full();
            items_.erase(items_.begin());
            if(! items_.empty())
              (*items_.front())();
            return was_full;
          }

          // Called by the HTTP handler to send a response.
          template<bool isRequest, class Body, class Fields>
          void
          operator()(http::message<isRequest, Body, Fields>&& msg)
          {
              // This holds a work item
              struct work_impl : work
              {
                HttpSession& self_;
                http::message<isRequest, Body, Fields> msg_;

                work_impl(
                    HttpSession& self,
                    http::message<isRequest, Body, Fields>&& msg)
                : self_(self)
                , msg_(std::move(msg))
                {
                }

                void
                operator()()
                {
                  http::async_write(
                      self_.derived().stream(),
                      msg_,
                      boost::asio::bind_executor(
                          self_.strand_,
                          std::bind(
                              &HttpSession::onWrite,
                              self_.derived().shared_from_this(),
                              std::placeholders::_1,
                              msg_.need_eof())));
                }
              };

              // Allocate and store the work
              items_.push_back(
                  boost::make_unique<work_impl>(self_, std::move(msg)));

              // If there was no previous work, start this one
              if(items_.size() == 1)
                (*items_.front())();
          }
      };

      std::string const& doc_root_;
      http::request<http::string_body> req_;
      queue queue_;

    protected:
      boost::asio::steady_timer timer_;
      boost::asio::strand<
      boost::asio::io_context::executor_type> strand_;
      boost::beast::flat_buffer bufferRead_;
      RequestHandler requestHandler_;
      WsChannel channel;

    public:
      // Construct the session
      HttpSession(
          boost::asio::io_context& ioc,
          boost::beast::flat_buffer buffer,
          std::string const& doc_root,
          RequestHandler requestHandler)
    : doc_root_(doc_root)
    , queue_(*this)
    , timer_(ioc,
        (std::chrono::steady_clock::time_point::max)())
    , strand_(ioc.get_executor())
    , bufferRead_(std::move(buffer))
    , requestHandler_(requestHandler)
    {
    }

      void
      doRead()
      {
        // Set the timer
        timer_.expires_after(std::chrono::seconds(HTTP_TIMEOUT_VALUE));

        // Make the request empty before reading,
        // otherwise the operation behavior is undefined.
        req_ = {};

        // Read a request
        http::async_read(
            derived().stream(),
            bufferRead_,
            req_,
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &HttpSession::onRead,
                    derived().shared_from_this(),
                    std::placeholders::_1)));
      }

      // Called when the timer expires.
      void
      onTimer(boost::system::error_code ec)
      {
        if(ec && ec != boost::asio::error::operation_aborted) {
          fail<>(&derived(), ec, "timer");
          return;
        }

        // Verify that the timer really expired since the deadline may have moved.
        if(timer_.expiry() <= std::chrono::steady_clock::now())
          return derived().doTimeout();

        // Wait on the timer
        timer_.async_wait(
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &HttpSession::onTimer,
                    derived().shared_from_this(),
                    std::placeholders::_1)));
      }

      void
      onRead(boost::system::error_code ec)
      {
        // Happens when the timer closes the socket
        if(ec == boost::asio::error::operation_aborted)
          return;

        // This means they closed the connection
        if(ec == http::error::end_of_stream)
          return derived().doEof();

        if(ec)
          fail(ec, "read");

        // See if it is a WebSocket Upgrade
        if(websocket::is_upgrade(req_))
        {
          // Transfer the stream to a new WebSocket session
          return makeWebsocketSession(
              derived().release_stream(),
              std::move(req_), requestHandler_);
        }

        // Otherwise handle pure HTTP connection
        std::string jsonRequest;

        // if we got correct JSON request back, send it to handler
        if (rest2json->GetJson(std::string(req_.method_string()),
                               std::string(req_.target()),
                               jsonRequest))
        {
          requestHandler_(jsonRequest, channel);
        }
        else
        {
          // write jsonRequest back as error response
          //queue_(bad_request(jsonRequest));
        }

        // If we aren't at the queue limit, try to pipeline another request
        if(! queue_.is_full())
          doRead();
      }

      void
      onWrite(boost::system::error_code ec, bool close)
      {
        // Happens when the timer closes the socket
        if(ec == boost::asio::error::operation_aborted)
          return;// provide error JSON

        if(ec) {
          fail<>(&derived(), ec, "write");
          return;
        }

        if(close)
        {
          // This means we should close the connection, usually because
          // the response indicated the "Connection: close" semantic.
          return derived().doEof();
        }

        // Inform the queue that a write completed
        if(queue_.onWrite())
        {
          // Read another request
          doRead();
        }
      }
  };

  // Handles a plain HTTP connection
  class PlainHttpSession
      : public HttpSession<PlainHttpSession>
  , public std::enable_shared_from_this<PlainHttpSession>
  {
      tcp::socket socket_;
      boost::asio::strand<
      boost::asio::io_context::executor_type> strand_;

    public:
      // Create the http_session
      PlainHttpSession(
          tcp::socket socket,
          boost::beast::flat_buffer buffer,
          std::string const& doc_root,
          RequestHandler requestHandler)
    : HttpSession<PlainHttpSession>(
        socket.get_executor().context(),
        std::move(buffer),
        doc_root,
        requestHandler)
        , socket_(std::move(socket))
        , strand_(socket_.get_executor())
        {
        }

      // Called by the base class
      tcp::socket&
      stream()
      {
        return socket_;
      }

      // Called by the base class
      tcp::socket
      release_stream()
      {
        return std::move(socket_);
      }

      // Start the asynchronous operation
      void
      run()
      {
        channel = connHandler.AddClient(this);

        // Run the timer. The timer is operated
        // continuously, this simplifies the code.
        onTimer({});

        doRead();
      }

      void
      doEof()
      {
        // Send a TCP shutdown
        boost::system::error_code ec;
        socket_.shutdown(tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
      }

      void
      doTimeout()
      {
        // Closing the socket cancels all outstanding operations. They
        // will complete with boost::asio::error::operation_aborted
        boost::system::error_code ec;
        socket_.shutdown(tcp::socket::shutdown_both, ec);
        socket_.close(ec);
      }
  };

  // Handles an SSL HTTP connection
  class SslHttpSession
      : public HttpSession<SslHttpSession>
  , public std::enable_shared_from_this<SslHttpSession>
  {
      ssl_stream<tcp::socket> stream_;
      boost::asio::strand<
      boost::asio::io_context::executor_type> strand_;
      bool eof_ = false;

    public:
      // Create the http_session
      SslHttpSession(
          tcp::socket socket,
          ssl::context& ctx,
          boost::beast::flat_buffer buffer,
          std::string const& doc_root,
          RequestHandler requestHandler)
    : HttpSession<SslHttpSession>(
        socket.get_executor().context(),
        std::move(buffer),
        doc_root,
        requestHandler)
        , stream_(std::move(socket), ctx)
        , strand_(stream_.get_executor())
        {
        }

      // Called by the base class
      ssl_stream<tcp::socket>&
      stream()
      {
        return stream_;
      }

      // Called by the base class
      ssl_stream<tcp::socket>
      release_stream()
      {
        return std::move(stream_);
      }

      // Start the asynchronous operation
      void
      run()
      {
        channel = connHandler.AddClient(this);

        // Run the timer. The timer is operated
        // continuously, this simplifies the code.
        /* TODO: There is a crash when using SSL to communicate
         * when line below is un-commented.. Evaluate later
         */
        //onTimer({});

        // Set the timer
        timer_.expires_after(std::chrono::seconds(HTTP_TIMEOUT_VALUE));

        // Perform the SSL handshake
        // Note, this is the buffered version of the handshake.
        stream_.async_handshake(
            ssl::stream_base::server,
            bufferRead_.data(),
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &SslHttpSession::onHandshake,
                    shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2)));
      }
      void
      onHandshake(
          boost::system::error_code ec,
          std::size_t bytes_used)
      {
        // Happens when the handshake times out
        if(ec == boost::asio::error::operation_aborted)
          return;

        if(ec)
          return fail(ec, "handshake");

        // Consume the portion of the buffer used by the handshake
        bufferRead_.consume(bytes_used);

        doRead();
      }

      void
      doEof()
      {
        eof_ = true;

        // Set the timer
        timer_.expires_after(std::chrono::seconds(HTTP_TIMEOUT_VALUE));

        // Perform the SSL shutdown
        stream_.async_shutdown(
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &SslHttpSession::onShutdown,
                    shared_from_this(),
                    std::placeholders::_1)));
      }

      void
      onShutdown(boost::system::error_code ec)
      {
        // Happens when the shutdown times out
        if(ec == boost::asio::error::operation_aborted)
          return;

        if(ec)
          return fail(ec, "shutdown");

        connHandler.RemoveClient(this);
        // At this point the connection is closed gracefully
      }

      void
      doTimeout()
      {
        // If this is true it means we timed out performing the shutdown
        if(eof_)
          return;

        // Start the timer again
        timer_.expires_at(
            (std::chrono::steady_clock::time_point::max)());
        onTimer({});
        //doEof();
      }
  };

  //------------------------------------------------------------------------------
  // Detects SSL handshakes
  class DetectSession : public std::enable_shared_from_this<DetectSession>
  {
      tcp::socket socket_;
      ssl::context& ctx_;
      boost::asio::strand<
      boost::asio::io_context::executor_type> strand_;
      std::string const& doc_root_;
      boost::beast::flat_buffer buffer_;
      RequestHandler requestHandler_;

    public:
      explicit
      DetectSession(
          tcp::socket socket,
          ssl::context& ctx,
          std::string const& doc_root,
          RequestHandler requestHandler)
      : socket_(std::move(socket))
      , ctx_(ctx)
      , strand_(socket_.get_executor())
      , doc_root_(doc_root)
      , requestHandler_(requestHandler)
      {
      }

      // Launch the detector
      void
      run()
      {
        async_detect_ssl(
            socket_,
            buffer_,
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &DetectSession::onDetect,
                    shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2)));
      }

      void
      onDetect(boost::system::error_code ec, boost::tribool result)
      {
        if(ec)
          return fail(ec, "detect");

        if(result)
        {
          // Launch SSL session
          std::make_shared<SslHttpSession>(
              std::move(socket_),
              ctx_,
              std::move(buffer_),
              doc_root_,
              requestHandler_)->run();
          return;
        }

        if (allowInsecureConns)
        {
          // Launch plain session
          std::make_shared<PlainHttpSession>(
              std::move(socket_),
              std::move(buffer_),
              doc_root_,
              requestHandler_)->run();
        }
        else
        {
          logger->Log(LogLevel::ERROR, "Plain (non-SSL) connection not allowed. Stopping connection.");
        }
      }
  };

  // Accepts incoming connections and launches the sessions
  class BeastListener : public std::enable_shared_from_this<BeastListener>
  {
      ssl::context& ctx_;
      tcp::acceptor acceptor_;
      tcp::socket socket_;
      std::string const& doc_root_;
      RequestHandler requestHandler_;

    public:
      BeastListener(
          boost::asio::io_context& ioc,
          ssl::context& ctx,
          tcp::endpoint endpoint,
          std::string const& doc_root,
          RequestHandler requestHandler)
    : ctx_(ctx)
    , acceptor_(ioc)
    , socket_(ioc)
    , doc_root_(doc_root)
    , requestHandler_(requestHandler)
    {
        boost::system::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if(ec)
        {
          fail(ec, "open");
          return;
        }

        // Allow address reuse
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
        if(ec)
        {
          fail(ec, "set_option");
          return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if(ec)
        {
          fail(ec, "bind");
          return;
        }

        // Start listening for connections
        acceptor_.listen(
            boost::asio::socket_base::max_listen_connections, ec);
        if(ec)
        {
          fail(ec, "listen");
          return;
        }
    }

      // Start accepting incoming connections
      void
      run()
      {
        if(! acceptor_.is_open())
          return;
        doAccept();
      }

      void
      doAccept()
      {
        acceptor_.async_accept(
            socket_,
            std::bind(
                &BeastListener::onAccept,
                shared_from_this(),
                std::placeholders::_1));
      }

      void
      onAccept(boost::system::error_code ec)
      {
        if(ec)
        {
          fail(ec, "accept");
        }
        else
        {
          // Create the detector http_session and run it
          std::make_shared<DetectSession>(
              std::move(socket_),
              ctx_,
              doc_root_,
              requestHandler_)->run();
        }

        // Accept another connection
        doAccept();
      }
  };
} // end namespace


WebSockHttpFlexServer::WebSockHttpFlexServer(std::shared_ptr<ILogger> loggerUtil,
                                             std::shared_ptr<IRestHandler> rest2jsonUtil)
 : logger_(loggerUtil),
   rest2json_(rest2jsonUtil) {
  logger = logger_;
  rest2json = rest2json_;
}

WebSockHttpFlexServer::~WebSockHttpFlexServer() {
  ioc->stop(); // stop execution of io runner

  // wait to finish
  for(auto& thread : iocRunners) {
    thread.join();
  }
}
void WebSockHttpFlexServer::Initialize(std::string host, int port, std::string && docRoot, bool allowInsecure) {
    logger_->Log(LogLevel::INFO, "Initializing Boost.Beast web-socket and http server");

    docRoot_ = docRoot;

    allowInsecureConns = allowInsecure;

    ioc = std::make_shared<boost::asio::io_context>(NumOfThreads);
    ctx.set_options(ssl::context::default_workarounds);

    boost::asio::ip::tcp::resolver resolver{*ioc};
    boost::asio::ip::tcp::resolver::query query(host, to_string(port));
    boost::asio::ip::tcp::resolver::iterator resolvedHost = resolver.resolve(query);

    // load required certificates for SSL connections
    LoadCertData(ctx);

    // handling function redirecting requests to registered listeners
    RequestHandler reqHndl = std::bind(&WebSockHttpFlexServer::HandleRequest,
                                       this,
                                       std::placeholders::_1,
                                       std::placeholders::_2);

    // create listener for handling incoming connections
    connListener = std::make_shared<BeastListener>(
      *ioc,
      ctx,
      resolvedHost->endpoint(),
      std::move(docRoot_),
      std::bind(&WebSockHttpFlexServer::HandleRequest,
                this,
                std::placeholders::_1,
                std::placeholders::_2));
}

std::string WebSockHttpFlexServer::HandleRequest(const std::string &req_json, WsChannel &channel) {
  std::string response;
  auto const type = channel.getType();
  ObserverType handlerType;

  if ((type == WsChannel::Type::WEBSOCKET_PLAIN) ||
      (type == WsChannel::Type::WEBSOCKET_SSL ))
  {
    handlerType = ObserverType::WEBSOCKET;
  }
  else
  {
    handlerType = ObserverType::HTTP;
  }

  for (auto const& handler : listeners_)
  {
    if ((handler.first == ObserverType::ALL) || (handler.first == handlerType))
    {
      response = handler.second->processQuery(req_json, channel);
    }
  }

  return response;
}

void WebSockHttpFlexServer::AddListener(ObserverType type,
                                        std::shared_ptr<IVssCommandProcessor> listener) {
  listeners_.push_back(std::make_pair(type, listener));
}

void WebSockHttpFlexServer::RemoveListener(ObserverType type,
                                           std::shared_ptr<IVssCommandProcessor> listener) {
  auto listenerToRemove = std::make_pair(type, listener);

  for (auto item = std::begin(listeners_); item < std::end(listeners_); ++item)
  {
    if (*item == listenerToRemove)
    {
      listeners_.erase(item);
      return;
    }
  }

  logger_->Log(LogLevel::WARNING, "Could not find listener to remove. Ignoring...");
}

void WebSockHttpFlexServer::LoadCertData(boost::asio::ssl::context& ctx) {
  std::string cert;
  std::string key;
  std::string line;

  // read certicate
  {
    std::ifstream inputFile ("Server.pem");
    if (inputFile.is_open())
    {
      while (getline(inputFile, line) )
      {
        cert.append(line);
        cert.append("\n");
      }
      inputFile.close();
    }
  }
  // read key
  {
    std::ifstream inputFile ("Server.key");
    if (inputFile.is_open())
    {
      while (getline(inputFile, line) )
      {
        key.append(line);
        key.append("\n");
      }
      inputFile.close();
    }
  }

  // add cert and key to security context for using with new connections
  ctx.use_certificate_chain(
      boost::asio::buffer(cert.data(), cert.size()));

  ctx.use_private_key(
      boost::asio::buffer(key.data(), key.size()),
      boost::asio::ssl::context::file_format::pem);

  isInitialized = true;
}

void WebSockHttpFlexServer::SendToConnection(uint64_t connID, const std::string &message) {
  if (!isInitialized)
  {
    std::string err("Cannot send to connection, server not initialized!");
    logger_->Log(LogLevel::ERROR, err);
    throw std::runtime_error(err);
  }

  bool isFound = false;

  // try to find active connection to send data to

  if (!isFound) {
    auto session = reinterpret_cast<PlainWebsocketSession *>(connID);
    std::lock_guard<std::mutex> lock(connHandler.mPlainWebSock_);
    auto iter = connHandler.connPlainWebSock_.find(session);
    if (iter != std::end(connHandler.connPlainWebSock_))
    {
      isFound = true;
      session->write(message);
    }
  }
  if (!isFound) {
    auto session = reinterpret_cast<SslWebsocketSession *>(connID);
    std::lock_guard<std::mutex> lock(connHandler.mSslWebSock_);
    auto iter = connHandler.connSslWebSock_.find(session);
    if (iter != std::end(connHandler.connSslWebSock_))
    {
      isFound = true;
      session->write(message);
    }
  }
  if (!isFound) {
    auto session = reinterpret_cast<PlainHttpSession *>(connID);
    std::lock_guard<std::mutex> lock(connHandler.mPlainHttp_);
    auto iter = connHandler.connPlainHttp_.find(session);
    if (iter != std::end(connHandler.connPlainHttp_))
    {
      isFound = true;
      // TODO: check how we are going to handle ASYNC writes on HTTP connection?
    }
  }
  if (!isFound) {
    auto session = reinterpret_cast<SslHttpSession *>(connID);
    std::lock_guard<std::mutex> lock(connHandler.mSslHttp_);
    auto iter = connHandler.connSslHttp_.find(session);
    if (iter != std::end(connHandler.connSslHttp_))
    {
      isFound = true;
      // TODO: check how we are going to handle ASYNC writes on HTTP connection?
    }
  }
}

void WebSockHttpFlexServer::Start() {
  if (!isInitialized)
  {
    std::string err("Cannot start server, server not initialized!");
    logger_->Log(LogLevel::ERROR, err);
    throw std::runtime_error(err);
  }

  logger_->Log(LogLevel::INFO, "Starting Boost.Beast web-socket and http server");

  // start listening for connections
  connListener->run();

  // run the I/O service on the requested number of threads
  iocRunners.reserve(NumOfThreads);
  for(auto i = 0; i < NumOfThreads; ++i) {
    iocRunners.emplace_back(
      []
      {
        boost::system::error_code ec;
        ioc->run(ec);
      });
  }
}
