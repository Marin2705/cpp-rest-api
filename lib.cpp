// freely modified from a Boost.Beast example :
//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: HTTP server, synchronous
//
//------------------------------------------------------------------------------

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/config.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

template <
    class Body, class Allocator,
    class Send>
void handle_request(
    http::request<Body, http::basic_fields<Allocator>> &&req,
    Send &&send,
    std::vector<Controller> router)
{
    // Returns a bad request response
    auto const bad_request =
        [&req](beast::string_view why)
    {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
    // auto const server_error =
    //     [&req](beast::string_view what)
    // {
    //     http::response<http::string_body> res{http::status::internal_server_error, req.version()};
    //     res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    //     res.set(http::field::content_type, "text/html");
    //     res.keep_alive(req.keep_alive());
    //     res.body() = "An error occurred: '" + std::string(what) + "'";
    //     res.prepare_payload();
    //     return res;
    // };

    // Make sure we can handle the method
    // if (req.method() != http::verb::get &&
    //     req.method() != http::verb::head)
    //     return send(bad_request("Unknown HTTP-method"));

    std::vector<Controller>::iterator it;
    for (it = router.begin(); it != router.end(); it++)
    {
        if (req.target() == it->getRoute())
        {
            // Respond to GET request
            http::response<http::string_body> res{http::status::bad_request, req.version()};
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = std::string(it->getResponse());
            res.prepare_payload();
            return send(std::move(res));
        }
    }

    return send(bad_request("Illegal request-target"));
}

//------------------------------------------------------------------------------

// Report a failure
void fail(beast::error_code ec, char const *what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// This is the C++11 equivalent of a generic lambda.
// The function object is used to send an HTTP message.
template <class Stream>
struct send_lambda
{
    Stream &stream_;
    bool &close_;
    beast::error_code &ec_;

    explicit send_lambda(
        Stream &stream,
        bool &close,
        beast::error_code &ec)
        : stream_(stream), close_(close), ec_(ec)
    {
    }

    template <bool isRequest, class Body, class Fields>
    void
    operator()(http::message<isRequest, Body, Fields> &&msg) const
    {
        // Determine if we should close the connection after
        close_ = msg.need_eof();

        // We need the serializer here because the serializer requires
        // a non-const file_body, and the message oriented version of
        // http::write only works with const messages.
        http::serializer<isRequest, Body, Fields> sr{msg};
        http::write(stream_, sr, ec_);
    }
};

// Handles an HTTP server connection
void do_session(tcp::socket &socket, std::vector<Controller> router)
{
    bool close = false;
    beast::error_code ec;

    // This buffer is required to persist across reads
    beast::flat_buffer buffer;

    // This lambda is used to send messages
    send_lambda<tcp::socket> lambda{socket, close, ec};

    for (;;)
    {
        // Read a request
        http::request<http::string_body> req;
        http::read(socket, buffer, req, ec);
        if (ec == http::error::end_of_stream)
            break;
        if (ec)
            return fail(ec, "read");

        // Send the response
        handle_request(std::move(req), lambda, router);
        if (ec)
            return fail(ec, "write");
        if (close)
        {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            break;
        }
    }

    // Send a TCP shutdown
    socket.shutdown(tcp::socket::shutdown_send, ec);

    // At this point the connection is closed gracefully
}

//------------------------------------------------------------------------------

int start(int argc, char *argv[], std::vector<Controller> router)
{
    try
    {
        // Check command line arguments.
        if (argc != 3)
        {
            std::cerr << "Usage: http-server-sync <address> <port>\n"
                      << "Example:\n"
                      << "    http-server-sync 0.0.0.0 8080 .\n";
            return EXIT_FAILURE;
        }
        auto const address = net::ip::make_address(argv[1]);
        auto const port = static_cast<unsigned short>(std::atoi(argv[2]));

        // The io_context is required for all I/O
        net::io_context ioc{1};

        // The acceptor receives incoming connections
        tcp::acceptor acceptor{ioc, {address, port}};
        for (;;)
        {
            // This will receive the new connection
            tcp::socket socket{ioc};

            // Block until we get a connection
            acceptor.accept(socket);

            // Launch the session, transferring ownership of the socket
            std::thread{std::bind(
                            &do_session,
                            std::move(socket),
                            router)}
                .detach();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}