# beast_machine
A test friendly state machine for beast applications

Please note this project is young and needs some polish, please send feedback on improvements.  Currently only tested on clang.

## Introduction

There are numerous state machines implemented for C++, however when you are making an app where both sides are on different machines, it is hard to test.  

This library is designed to allow you to create a state engine that matches your sequence diagram in code.  You conditionally compile your state engine as server, client or unit_test, this will then activate different sections of your code according to what you want to do.

The state machine is capable of supporting nested state machines for more complicated interaction.

The state machine does not specify the payload going between your client and server, I would recommend that you use something like https://google.github.io/flatbuffers/ for this.

The state machine supports individual messages and streamed data.

The test project has the following sequence:

![Sequence diagram](https://github.com/edwardbr/beast_machine/blob/master/docs/flow_diagram.png?raw=true)

## Build instructions

The required library is header only so knock yourself out and just copy the three headers.  

The package supports cmake and Hunter package manager (though not in the repo yet), please read the docs on this.  If you want an example project see https://github.com/edwardbr/hunterprimer.

However in summary using cmake add a cmake/Hunter/config.cmake file:

```
hunter_config(beast_machine
    URL https://github.com/edwardbr/beast_machine/archive/v1.0.tar.gz
    SHA1 228dea840b14e1894e97f0cd98466693bfdf2aed)
```
Where the SHA code is whatever is required, ideally please use a tagged branch.

Then in your CMakeLists.txt file add

```
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include(HunterGate)

HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.23.214.tar.gz"
    SHA1 "e14bc153a7f16d6a5eeec845fb0283c8fad8c358"
    LOCAL
)

hunter_add_package(beast_machine)
find_package(beast_machine CONFIG REQUIRED)

# dont forget to add beast_machine to your project
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        beast_machine::beast_machine
)
```

Alternatively you can clone the repo and build and run the test.

The state engine you write yourself is little more than an encapsulated switch statement with if constexprs for each state transistion server client combination providing a simple coroutine type construct.


```

beast_machine::callback_return callback(beast::flat_buffer& buffer, size_t& readable_bytes,
                                                bool message_read_complete)
{
    switch (_state)
    {
    case initialise:
        _state = receive_hello;
        REQUIRE(readable_bytes == 0); // NOLINT
        // server is initialised
        if constexpr (is_server) // NOLINT
        {                        // NOLINT
            return beast_machine::callback_return(beast_machine::callback_result::read, std::string());
        }
        // client sends hello
        if constexpr (is_client) // NOLINT
        {                        // NOLINT
            return beast_machine::callback_return(beast_machine::callback_result::write_complete,
                                                  "hello world");
        }
        break;
...
```

Each case statement can then update the state of the engine according to your requirements, making sure that your client and server remain in lockstep.

Your switch statement then controls the state of the underlying beast_machine runtime state engine with these values:

```
enum class callback_result
{
    read,              //async read full blob
    need_more_reading, //async read little bits of blob
    need_more_writing, //async write little bits of blob
    write_complete,    //complete writing and then start reading
    write_complete_async_read,    //complete writing and then start reading little bits
    close              //terminate the conversation
};
```

You need to set initial states for each:

The server should be beast_machine::callback_result::read

And the client to beast_machine::callback_result::write_complete, or need_more_writing, or write_complete_async_read.

Either side can choose to end the conversation by setting the callback result to beast_machine::callback_result::close.
