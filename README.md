# beast_machine
A test friendly state machine for beast applications

Please note this project is young and needs some polish, please send feedback on improvements.  Currently only tested on clang.

## Introduction

There are numerous state machines implemented for C++, however when you are making an app where both sides are on different machines, it is hard to test.  

This library is designed to allow you to create a state engine that matches your sequence diagram in code.  You conditionally compile your state engine as server, client or unit_test, this will then activate different sections of your code according to what you want to do.

The state machine is capable of supporting nested state machines for more complicated interaction.

The state machine does not specify the payload going between your client and server, I would recommend that you use something like googles flatbuffer for this.

The state machine supports individual messages and streamed data.

The test project has the following sequence:

![Sequence diagram](https://github.com/edwardbr/beast_machine/blob/master/docs/flow_diagram.png?raw=true)

## Build instructions

The required library is header only so knock yourself out and just copy the three headers.  

The package supports cmake and Hunter package manager (though not in the repo yet), please read the docs on this.  If you want an example project see https://github.com/edwardbr/hunterprimer.

However in summary using cmake add a cmake/Hunter/config.cmake file:

```
hunter_config(beast_machine
    URL https://github.com/edwardbr/beast_machine/archive/master.tar.gz
    SHA1 08b9d72534b34885139bb3dad74839b6b475a1ab)
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
