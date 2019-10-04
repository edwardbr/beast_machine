#pragma once

#include <system_error>

namespace beast_machine
{
//this enum type expects that there will be no overlapped communication between sender and receiver
//a sender will complete sending before it starts receiving
enum class callback_result
{
    read,              //async read full blob
    need_more_reading, //async read little bits of blob
    need_more_writing, //async write little bits of blob
    write_complete,    //complete writing and then start reading
    write_complete_async_read,    //complete writing and then start reading little bits
    close              //terminate the conversation
};

using callback_return = std::tuple<callback_result, std::string>;


enum class errc
{
  // no 0
  unrecognised_state = 1, 
  unexpected_exception, 
};

std::error_code make_error_code(beast_machine::errc);
}

namespace std
{
  template <>
    struct is_error_code_enum<beast_machine::errc> : true_type {};
}
 
