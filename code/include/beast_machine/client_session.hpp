#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include "session.hpp"

namespace beast_machine
{
namespace client
{

    namespace beast = boost::beast;         // from <boost/beast.hpp>
    namespace http = beast::http;           // from <boost/beast/http.hpp>
    namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
    namespace net = boost::asio;            // from <boost/asio.hpp>
    using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

    // Sends a WebSocket message and prints the response
    template<class T, class FailSync>
    class session : public std::enable_shared_from_this<session<T, FailSync>>, public T
    {
        tcp::resolver _resolver;
        websocket::stream<beast::tcp_stream> _ws;
        beast::flat_buffer _buffer;
        std::string _host;
        std::string _target;
        std::shared_ptr<FailSync> _fail_sync;

    public:

        // Resolver and socket require an io_context
        explicit session(net::io_context& ioc, std::shared_ptr<FailSync> fs)
            : _resolver(net::make_strand(ioc))
            , _ws(net::make_strand(ioc))
            , _fail_sync(fs)
        {
        }

        // Start the asynchronous operation
        void run(char const* host, char const* port, char const* target)
        {
            // Save these for later
            _host = host;
            _target = target;

            // Look up the domain name
            _resolver.async_resolve(
                host, port, beast::bind_front_handler(&session::on_resolve, session<T, FailSync>::shared_from_this()));
        }

        void on_resolve(beast::error_code ec, tcp::resolver::results_type results)
        {
            if (ec)
                return fail(ec, "resolve");

            // Set the timeout for the operation
            beast::get_lowest_layer(_ws).expires_after(std::chrono::seconds(30));

            // Make the connection on the IP address we get from a lookup
            beast::get_lowest_layer(_ws).async_connect(
                results, beast::bind_front_handler(&session::on_connect, session<T, FailSync>::shared_from_this()));
        }

        void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
        {
            if (ec)
                return fail(ec, "connect");

            // Turn off the timeout on the tcp_stream, because
            // the websocket stream has its own timeout system.
            beast::get_lowest_layer(_ws).expires_never();

            // Set suggested timeout settings for the websocket
            _ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));

            // Set a decorator to change the User-Agent of the handshake
            _ws.set_option(websocket::stream_base::decorator([](websocket::request_type& req) {
                req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async");
            }));

            // Perform the websocket handshake
            _ws.async_handshake(
                _host, _target,
                beast::bind_front_handler(&session::on_handshake, session<T, FailSync>::shared_from_this()));
        }

        void process_message(size_t bytes)
        {
            try
            {
                callback_return ret = T::callback(_buffer, bytes, _ws.is_message_done());

                _buffer.consume(_buffer.size());

                switch (std::get<0>(ret))
                {
                case callback_result::need_more_reading:
                    // Read a message into our buffer
                    do_read_blob();
                    return;
                case callback_result::need_more_writing:
                    // Send the message
                    _ws.async_write_some(false, net::buffer(std::get<1>(ret)),
                                         beast::bind_front_handler(&session::on_write_contunue,
                                                                   session<T, FailSync>::shared_from_this()));
                    return;
                case callback_result::write_complete:
                    // Send the message
                    _ws.async_write_some(
                        true, net::buffer(std::get<1>(ret)),
                        beast::bind_front_handler(&session::on_write, session<T, FailSync>::shared_from_this()));
                    return;
                case callback_result::write_complete_async_read:
                    // Send the message
                    _ws.async_write_some(true, net::buffer(std::get<1>(ret)),
                                         beast::bind_front_handler(&session::on_write_complete_async_read,
                                                                   session<T, FailSync>::shared_from_this()));
                    return;
                case callback_result::close:
                    break;
                default:
                    throw std::logic_error("unexpected switch state");
                    break;
                }
            }
            catch (std::logic_error ex)
            {
                return fail(make_error_code(errc::unrecognised_state), ex.what());
            }
            catch (std::exception& ex)
            {
                return fail(make_error_code(errc::unexpected_exception), ex.what());
            }

            // Close the WebSocket connection
            _ws.async_close(websocket::close_code::normal,
                            beast::bind_front_handler(&session::on_close, session<T, FailSync>::shared_from_this()));
        }

        void on_handshake(beast::error_code ec)
        {
            if (ec)
                return fail(ec, "handshake");

            process_message(0);
        }

        void do_read_blob()
        {
            // Read a message into our buffer
            _ws.async_read_some(_buffer, _buffer.capacity(),
                                beast::bind_front_handler(&session::on_read, session<T, FailSync>::shared_from_this()));
        }

        void on_read(beast::error_code ec, std::size_t bytes_transferred)
        {
            REQUIRE(_buffer.size() == bytes_transferred); // NOLINT
            // boost::ignore_unused(bytes_transferred);

            if (ec)
                return fail(ec, "read");

            process_message(bytes_transferred);
        }

        void on_write(beast::error_code ec, std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);

            if (ec)
                return fail(ec, "write");

            // Read a message into our buffer
            _ws.async_read(_buffer,
                           beast::bind_front_handler(&session::on_read, session<T, FailSync>::shared_from_this()));
        }

        void on_write_complete_async_read(beast::error_code ec, std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);

            if (ec)
                return fail(ec, "write");

            // Read a message into our buffer
            do_read_blob();
        }

        void on_write_contunue(beast::error_code ec, std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);

            if (ec)
                return fail(ec, "write");

            process_message(0);
        }

        void on_close(beast::error_code ec)
        {
            if (ec)
                return fail(ec, "close");
        }

    private:
        template<class err_code> void fail(err_code ec, char const* what) { (*_fail_sync)(ec, what); }
    };
} // namespace client
} // namespace beast_machine
