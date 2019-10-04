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
    namespace server
    {

        namespace beast = boost::beast;         // from <boost/beast.hpp>
        namespace http = beast::http;           // from <boost/beast/http.hpp>
        namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
        namespace net = boost::asio;            // from <boost/asio.hpp>
        using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

        //------------------------------------------------------------------------------

        // Echoes back all received WebSocket messages
        template<class T, class FailSync>
        class session : public std::enable_shared_from_this<session<T, FailSync>>, public T
        {
            websocket::stream<beast::tcp_stream> ws_;
            beast::flat_buffer buffer_;
            std::shared_ptr<FailSync> fail_sync;

        public:
            // Take ownership of the socket
            explicit session(tcp::socket&& socket, std::shared_ptr<FailSync>& fs)
                : ws_(std::move(socket))
                , fail_sync(fs)
            {
            }

            // Start the asynchronous operation
            void run()
            {
                // Set suggested timeout settings for the websocket
                ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

                // Set a decorator to change the Server of the handshake
                ws_.set_option(websocket::stream_base::decorator([](websocket::response_type& res) {
                    res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async");
                }));

                // Accept the websocket handshake
                ws_.async_accept(beast::bind_front_handler(&session::on_accept, session<T, FailSync>::shared_from_this()));
            }

            void process_message(size_t bytes)
            {
                callback_return ret = T::callback(buffer_, bytes, ws_.is_message_done());

                buffer_.consume(buffer_.size());

                try
                {
                    switch (std::get<0>(ret))
                    {
                    case callback_result::read:
                        // Read a message into our buffer
                        do_read();
                        return;
                    case callback_result::need_more_reading:
                        // Read a message into our buffer
                        do_read_blob();
                        return;
                    case callback_result::need_more_writing:
                        // Send the message
                        ws_.async_write_some(false, net::buffer(std::get<1>(ret)),
                                            beast::bind_front_handler(&session::on_write_contunue,
                                                                    session<T, FailSync>::shared_from_this()));
                        return;
                    case callback_result::write_complete:
                        // Send the message
                        ws_.async_write_some(
                            true, net::buffer(std::get<1>(ret)),
                            beast::bind_front_handler(&session::on_write, session<T, FailSync>::shared_from_this()));
                        return;
                    case callback_result::write_complete_async_read:
                        // Send the message
                        ws_.async_write_some(true, net::buffer(std::get<1>(ret)),
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
                ws_.async_close(websocket::close_code::normal,
                                beast::bind_front_handler(&session::on_close, session<T, FailSync>::shared_from_this()));
            }

            void on_accept(beast::error_code ec)
            {
                if (ec)
                    return fail(ec, "accept");

                // tell the business logic we have accepted a connection request
                process_message(0);
            }

            void do_read()
            {
                // Read a message into our buffer
                ws_.async_read(buffer_,
                            beast::bind_front_handler(&session::on_read, session<T, FailSync>::shared_from_this()));
            }

            void do_read_blob()
            {
                // Read a message into our buffer
                ws_.async_read_some(buffer_, buffer_.capacity(),
                                    beast::bind_front_handler(&session::on_read, session<T, FailSync>::shared_from_this()));
            }

            void on_read(beast::error_code ec, std::size_t bytes_transferred)
            {
                boost::ignore_unused(bytes_transferred);

                // This indicates that the session was closed
                if (ec == websocket::error::closed)
                    return;

                if (ec)
                    fail(ec, "read");

                process_message(bytes_transferred);
            }

            void on_write(beast::error_code ec, std::size_t bytes_transferred)
            {
                boost::ignore_unused(bytes_transferred);

                if (ec)
                    return fail(ec, "write");

                // Read a message into our buffer
                do_read();
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
            template<class err_code> void fail(err_code ec, char const* what) { (*fail_sync)(ec, what); }
        };

        //------------------------------------------------------------------------------

        // Accepts incoming connections and launches the sessions
        template<class T, class FailSync> class listener : public std::enable_shared_from_this<listener<T, FailSync>>
        {
            net::io_context& ioc_;
            tcp::acceptor acceptor_;
            bool single_request_;
            std::shared_ptr<FailSync> fail_sync;

        public:
            listener(net::io_context& ioc, tcp::endpoint endpoint, bool single_request, std::shared_ptr<FailSync>& fs)
                : ioc_(ioc)
                , acceptor_(ioc)
                , single_request_(single_request)
                , fail_sync(fs)
            {
                beast::error_code ec;

                // Open the acceptor
                acceptor_.open(endpoint.protocol(), ec);
                if (ec)
                {
                    fail(ec, "open");
                    return;
                }

                // Allow address reuse
                acceptor_.set_option(net::socket_base::reuse_address(true), ec);
                if (ec)
                {
                    fail(ec, "set_option");
                    return;
                }

                // Bind to the server address
                acceptor_.bind(endpoint, ec);
                if (ec)
                {
                    fail(ec, "bind");
                    return;
                }

                // Start listening for connections
                acceptor_.listen(net::socket_base::max_listen_connections, ec);
                if (ec)
                {
                    fail(ec, "listen");
                    return;
                }
            }

            // Start accepting incoming connections
            void run() { do_accept(); }

        private:
            void do_accept()
            {
                // The new connection gets its own strand
                acceptor_.async_accept(
                    net::make_strand(ioc_),
                    beast::bind_front_handler(&listener::on_accept, listener<T, FailSync>::shared_from_this()));
            }

            void on_accept(beast::error_code ec, tcp::socket socket)
            {
                if (ec)
                {
                    fail(ec, "accept");
                }
                else
                {
                    // Create the session and run it
                    std::make_shared<session<T, FailSync>>(std::move(socket), fail_sync)->run();
                }

                // Accept another connection
                if (!single_request_)
                {
                    do_accept();
                }
            }
            template<class err_code> void fail(err_code ec, char const* what) { (*fail_sync)(ec, what); }
        };
    } // namespace server

    struct errc_category : std::error_category
    {
        const char* name() const noexcept override;
        std::string message(int ev) const override;
    };

    const char* errc_category::name() const noexcept { return "websocket_server"; }

    std::string errc_category::message(int ev) const
    {
        switch (static_cast<errc>(ev))
        {
        case errc::unrecognised_state:
            return "unrecognised state";

        case errc::unexpected_exception:
            return "unexpected exception";

        default:
            return "(unrecognized error)";
        }
    }

    const errc_category the_category {};

    std::error_code make_error_code(beast_machine::errc e) { return {static_cast<int>(e), beast_machine::the_category}; }
} // namespace beast_machine
