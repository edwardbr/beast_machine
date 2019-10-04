# beast_machine
A test friendly state machine for beast applications

## Introduction

There are numerous state machines implemented for C++, however when you are making an app where both sides are on different machines, it is hard to test.  

This library is designed to allow you to create a state engine that matches your sequence diagram in code.  You conditionally compile your state engine as server, client or unit_test, this will then activate different sections of your code according to what you want to do.

The state machine is capable of supporting nested state machines for more complicated interaction.

The state machine does not specify the payload going between your client and server, I would recommend that you use something like googles flatbuffer for this.

The state machine supports individual messages and streamed data.

The test project has the following sequence:
![Sequence diagram](https://github.com/edwardbr/beast_machine/blob/master/docs/flow_diagram.png?raw=true)
