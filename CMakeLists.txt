cmake_minimum_required(VERSION 3.10)
project(imageio CXX)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(GenericOptions -Wall)
    set(ReleaseOptions -O2)
    set(AlwaysSmallOptions -Os)
    set(ProfilerOptions -fprofile-instr-generate)
    set(ProfilerLinkOptions -fprofile-instr-generate)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(GenericOptions -Wall)
    set(ReleaseOptions -O2)
    set(AlwaysSmallOptions -Os)
    set(ProfilerOptions -pg)
    set(ProfilerLinkOptions -pg)
#elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
#  # using Intel C++
#elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
#  # using Visual Studio C++
else()
    set(GenericOptions "")
    set(ReleaseOptions "")
endif()

if (UNIX AND NOT APPLE)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

enable_testing()

set(Sources common/JSONParsers.cpp)

add_executable(unittest common/unittest.cpp ${Sources})
target_include_directories(unittest SYSTEM PRIVATE /usr/local/include)
target_include_directories(unittest PRIVATE common)
target_compile_definitions(unittest PRIVATE TESTDOC_UNITTEST)
target_compile_features(unittest PRIVATE cxx_std_17)
target_compile_options(unittest PRIVATE ${GenericOptions})
if ("${CMAKE_BUILD_TYPE}" STREQUAL "RELEASE")
    target_compile_options(unittest PRIVATE ${ReleaseOptions})
endif()
if (UNIX AND NOT APPLE)
    target_link_libraries(unittest PRIVATE Threads::Threads)
endif()
add_test(NAME UnitTest COMMAND unittest)

set(Parsers readimage writeimage)
add_custom_target(parser COMMENT "Generate parsers for main programs.")
#add_dependencies(parser ${Parsers})
set(JSONProfiler common/BlockQueue.cpp common/InputChannel.cpp common/FileDescriptorInput.cpp common/JSONParsers.cpp)

list(TRANSFORM Parsers APPEND "_io.hpp" OUTPUT_VARIABLE ParserHpp)
add_custom_target(parsers
    COMMAND edicta -i ${CMAKE_CURRENT_LIST_DIR}/README.md -o specs ${Parsers}
    COMMAND ${CMAKE_CURRENT_LIST_DIR}/iogen -i specs
    BYPRODUCTS ${ParserHpp})

#add_executable(writeimage common/unittest.cpp ${Sources} )
#add_dependencies(writeimage parsers)
#target_include_directories(writeimage SYSTEM /usr/local/include)
#target_include_directories(writeimage PRIVATE common)
#target_compile_features(writeimage PRIVATE cxx_std_17)
#target_compile_options(writeimage PRIVATE ${GenericOptions} )
#if ("${CMAKE_BUILD_TYPE}" STREQUAL "RELEASE")
    #target_compile_options(writeimage PRIVATE ${ReleaseOptions} )
#endif()
#if (UNIX AND NOT APPLE)
    #target_link_libraries(writeimage PRIVATE Threads::Threads)
#endif()

set(Profilers readfloatarray readfloatarray2)
add_custom_target(profile COMMENT "Building programs to be run with profiler")
add_dependencies(profile ${Profilers})
set(JSONProfiler common/BlockQueue.cpp common/InputChannel.cpp common/FileDescriptorInput.cpp common/JSONParsers.cpp)

# Custom target that generates all profiler parsers at the same time.
list(TRANSFORM Profilers APPEND "_io.hpp" OUTPUT_VARIABLE GenHpp)
add_custom_target(generated
    COMMAND edicta -i ${CMAKE_CURRENT_LIST_DIR}/profile/profile.md -o specs ${Profilers}
    COMMAND ${CMAKE_CURRENT_LIST_DIR}/iogen -i specs
    BYPRODUCTS ${GenHpp})

add_executable(readfloatarray profile/readparse.cpp ${JSONProfiler})
add_dependencies(readfloatarray generated)
target_compile_definitions(readfloatarray PRIVATE HEADER=readfloatarray_io.hpp)
target_include_directories(readfloatarray SYSTEM PRIVATE /usr/local/include)
target_include_directories(readfloatarray PRIVATE common)
target_include_directories(readfloatarray PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_features(readfloatarray PRIVATE cxx_std_17)
target_compile_options(readfloatarray PRIVATE ${GenericOptions} ${ProfilerOptions})
target_link_libraries(readfloatarray PRIVATE ${ProfilerLinkOptions})
if ("${CMAKE_BUILD_TYPE}" STREQUAL "RELEASE")
    target_compile_options(readfloatarray PRIVATE ${ReleaseOptions})
endif()

add_executable(readfloatarray2 profile/readparse.cpp ${JSONProfiler})
add_dependencies(readfloatarray2 generated)
target_compile_definitions(readfloatarray2 PRIVATE HEADER=readfloatarray2_io.hpp)
target_include_directories(readfloatarray2 SYSTEM PRIVATE /usr/local/include)
target_include_directories(readfloatarray2 PRIVATE common)
target_include_directories(readfloatarray2 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_features(readfloatarray2 PRIVATE cxx_std_17)
target_compile_options(readfloatarray2 PRIVATE ${GenericOptions} ${ProfilerOptions})
target_link_libraries(readfloatarray2 PRIVATE ${ProfilerLinkOptions})
if ("${CMAKE_BUILD_TYPE}" STREQUAL "RELEASE")
    target_compile_options(readfloatarray2 PRIVATE ${ReleaseOptions})
endif()
