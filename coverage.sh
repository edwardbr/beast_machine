#!/bin/bash

mkdir -p coverage
rm -rf coverage/*

echo merging output
/usr/local/clang_8.0.0/bin/llvm-profdata merge \
                build/test/default.profraw \
                -o coverage/test.profdata

echo show results
/usr/local/clang_8.0.0/bin/llvm-profdata show coverage/test.profdata

echo generate html
/usr/local/clang_8.0.0/bin/llvm-cov \
                show \
                -format=html \
                -o \
                coverage \
                build/test/test_beast_machine \
                -instr-profile=coverage/test.profdata
# /usr/bin/firefox ./coverage/index.html &
