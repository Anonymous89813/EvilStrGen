cmake_minimum_required(VERSION 3.9.0)
project(EvilStrGen)
set(CMAKE_CXX_STANDARD 17)

include_directories(${PROJECT_SOURCE_DIR}/util ${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR})
add_executable(EvilStrGen EvilStrGen.cpp
        src/stringpiece.cpp
        util/util.h src/re2.cpp
        util/logging.h
        util/strutil.h
        util/strutil.cpp
        util/utf.h
        src/prog.h
        src/sparse_array.h
        src/pod_array.h
        src/sparse_set.h
        src/regexp.h
        src/regexp.cpp
        util/mutex.h
        src/walker-inl.h
        src/parse.cpp
        src/unicode_casefold.h
        src/unicode_casefold.cpp
        src/unicode_groups.h
        src/unicode_groups.cpp
        src/prog.cpp
        src/bitmap256.h
        src/bitmap256.cpp
        src/compile.cpp
        src/onepass.cpp
        src/nfa.cpp
        util/rune.cpp
        src/dfa.cpp
        util/mix.h
        src/bitstate.cpp
        src/simplify.cpp
        src/tostring.cpp
        src/perl_groups.cpp
        src/dump.cpp
        util/test.cpp
        util/test.h
        src/dfa.h
        src/create_evil_text/CreateEvilText.cpp)
target_link_libraries(EvilStrGen pthread)