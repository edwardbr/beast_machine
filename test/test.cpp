#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_CONSOLE_WIDTH 300

#include <chrono>
#include <thread>
#include <iomanip>
#include <iostream>
#include <thread>
#include <map>
#include <memory>
#include <sstream>

#include <catch2/catch.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/process.hpp>

#include <beast_machine/client_session.hpp>
#include <beast_machine/server_session.hpp>

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

namespace websocket_with_beast_server_combined
{

    // Report a failure
    struct fail
    {
        void operator()(std::error_code ec, char const* what) { std::cerr << what << ": " << ec.message() << "\n"; }
    };

    enum class environment
    {
        client,
        server,
        unit_test
    };

    template<environment env> 
    class hello_world_task
    {
        enum client_state_state
        {
            initialise,
            receive_hello,
            receive_hello_streamed,
            end
        } _state;

        int _stream_count = 100;
        bool _read_streamed_data = false;

    public:
        hello_world_task()
            : _state(initialise)
        {
        }

        //this makes the code easier to read when you have unit_test state too
        static constexpr bool is_server = env != environment::client; 
        static constexpr bool is_client = env != environment::server;

        network::callback_return callback(beast::flat_buffer& buffer, size_t& readable_bytes,
                                          bool message_read_complete)
        {
            std::cout << beast::make_printable(buffer.cdata()) << "\n";
            switch (_state)
            {
            case initialise:
                _state = receive_hello;
                REQUIRE(readable_bytes == 0); // NOLINT
                // server is initialised
                if constexpr (is_server)                // NOLINT
                {                                         // NOLINT
                    return network::callback_return(network::callback_result::read, std::string());
                }
                // client sends hello
                if constexpr (is_client)                  // NOLINT
                {                                         // NOLINT
                    return network::callback_return(network::callback_result::write_complete, "hello world");
                }
                break;
            case receive_hello:

                // server receives hello and replies
                if constexpr (is_server)                  // NOLINT
                {                                         // NOLINT
                    REQUIRE(readable_bytes != 0);          // NOLINT
                    _state = receive_hello_streamed;
                    return network::callback_return(network::callback_result::write_complete,
                                                    std::string("responding1 hello back"));
                }
                // client receives callback and sends bitz of data asynchonously
                if constexpr (is_client)                  // NOLINT
                {                                         // NOLINT
                    std::stringstream ss;
                    ss << std::setfill('0') << std::setw(5) << _stream_count;
                    auto continuation = network::callback_result::need_more_writing;
                    if (_stream_count == 0)
                    {
                        continuation = network::callback_result::write_complete_async_read;
                        _state = receive_hello_streamed;
                        _stream_count = 100;
                    }
                    else
                    {
                        _stream_count--;
                    }

                    return network::callback_return(continuation, ss.str());
                }

                break;
            case receive_hello_streamed:
            {
                bool valid
                    = (_stream_count == 100 && readable_bytes != 0) || (_stream_count != 100 && readable_bytes == 0);
                REQUIRE(valid == true); // NOLINT

                // server receives bitz data asynchonously
                if constexpr (is_server)                // NOLINT
                {                                         // NOLINT
                    if (_read_streamed_data == false)
                    {
                        _read_streamed_data = message_read_complete;
                        if (!message_read_complete)
                        {
                            return network::callback_return(network::callback_result::need_more_reading,
                                                            std::string());
                        }
                    }
                    // then spews out its response
                    std::stringstream ss;
                    ss << std::setfill('0') << std::setw(5) << "r" << _stream_count;
                    auto continuation = network::callback_result::need_more_writing;
                    if (_stream_count == 0)
                    {
                        _state = end;
                        continuation = network::callback_result::write_complete;
                    }
                    _stream_count--;
                    return network::callback_return(continuation, ss.str());
                }

                if constexpr (is_client)                  // NOLINT
                {                                         // NOLINT
                    // client processes server's response
                    auto continuation = network::callback_result::need_more_reading;
                    if (message_read_complete)
                    {
                        _state = end;
                        continuation = network::callback_result::close;
                    }

                    return network::callback_return(continuation, std::string(""));
                }
                break;
            }
            default:
                std::cerr << "error\n";
                REQUIRE(false); // NOLINT
                break;
            }
            REQUIRE(false); // NOLINT
            return network::callback_return(network::callback_result::close, std::string());
        }
    };

    TEST_CASE("websocket with beast server combined") // NOLINT
    {
        auto const address = net::ip::make_address("127.0.0.1");
        auto const port = 8080;
        auto const threads = 1;

        auto f = std::make_shared<fail>();

        // The io_context is required for all I/O
        net::io_context server_ioc {threads};

        // Create and launch a listening port
        std::make_shared<network::server::listener<hello_world_task<environment::server>, fail>>(
            server_ioc, tcp::endpoint {address, port}, true, f)
            ->run();

        std::thread t([&server_ioc] { server_ioc.run(); });

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // Check command line arguments.
        auto const host = "127.0.0.1";
        auto const portstr = "8080";
        auto const targetstr = "/";

        // The io_context is required for all I/O
        net::io_context client_ioc;

        // Launch the asynchronous operation
        std::make_shared<network::client::session<hello_world_task<environment::client>, fail>>(client_ioc, f)
            ->run(host, portstr, targetstr);

        // Run the I/O service. The call will return when
        // the socket is closed.
        client_ioc.run();
        t.join();
    };
}

