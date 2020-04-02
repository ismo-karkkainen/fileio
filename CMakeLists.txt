cmake_minimum_required(VERSION 3.10)
project(imageio CXX)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(GenericOptions -Wall)
    set(ReleaseOptions -O2)
    set(AlwaysSmallOptions -Os)
    set(ProfilerOptions -fprofile-instr-generate)
    set(ProfilerLinkOptions -fprofile-instr-generate)
    set(CxxStd -std=c++17)
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

set(Sources common/BlockQueue.cpp common/InputChannel.cpp common/FileDescriptorInput.cpp common/JSONParsers.cpp common/JSONWriters.cpp)

add_executable(unittest common/unittest.cpp ${Sources})
target_include_directories(unittest SYSTEM PRIVATE /usr/local/include)
target_include_directories(unittest PRIVATE common)
target_compile_definitions(unittest PRIVATE TESTDOC_UNITTEST)
target_compile_options(unittest PRIVATE ${CxxStd})
target_compile_options(unittest PRIVATE ${GenericOptions})
if ("${CMAKE_BUILD_TYPE}" STREQUAL "RELEASE")
    target_compile_options(unittest PRIVATE ${ReleaseOptions})
endif()
if (UNIX AND NOT APPLE)
    target_link_libraries(unittest PRIVATE Threads::Threads)
endif()
add_test(NAME UnitTest COMMAND unittest)

if (NOT DEFINED ENV{NO_TIFF})
    find_package(TIFF)
endif()
if (TIFF_FOUND)
    message(STATUS "TIFF supported.")
else()
    message(STATUS "No TIFF support.")
endif()
function(setup_tiff TARGETNAME)
    if (TIFF_FOUND)
        target_include_directories(${TARGETNAME} SYSTEM PRIVATE ${TIFF_INCLUDE_DIR})
        target_link_libraries(${TARGETNAME} PRIVATE ${TIFF_LIBRARY})
    else()
        target_compile_definitions(${TARGETNAME} PRIVATE NO_TIFF)
    endif()
endfunction()

function(setup_main_program TGTNAME)
    add_dependencies(${TGTNAME} parsers)
    target_include_directories(${TGTNAME} SYSTEM PRIVATE /usr/local/include)
    target_include_directories(${TGTNAME} PRIVATE common)
    target_include_directories(${TGTNAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    setup_tiff(${TGTNAME})
    target_compile_options(${TGTNAME} PRIVATE ${CxxStd})
    target_compile_options(${TGTNAME} PRIVATE ${GenericOptions})
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "RELEASE")
        target_compile_options(${TGTNAME} PRIVATE ${ReleaseOptions})
    endif()
    if (UNIX AND NOT APPLE)
        target_link_libraries(${TGTNAME} PRIVATE Threads::Threads)
    endif()
endfunction()

function(setup_profiling TGTNAME)
    add_dependencies(${TGTNAME} generated)
    target_compile_definitions(${TGTNAME} PRIVATE HEADER=${TGTNAME}_io.hpp)
    target_include_directories(${TGTNAME} SYSTEM PRIVATE /usr/local/include)
    target_include_directories(${TGTNAME} PRIVATE common)
    target_include_directories(${TGTNAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_compile_options(${TGTNAME} PRIVATE ${CxxStd})
    target_compile_options(${TGTNAME} PRIVATE ${GenericOptions} ${ProfilerOptions})
    target_link_libraries(${TGTNAME} PRIVATE ${ProfilerLinkOptions})
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "RELEASE")
        target_compile_options(${TGTNAME} PRIVATE ${ReleaseOptions})
    endif()
endfunction()

set(Programs readimage writeimage)
set(JSONProfiler common/BlockQueue.cpp common/InputChannel.cpp common/FileDescriptorInput.cpp common/JSONParsers.cpp)

list(TRANSFORM Programs APPEND "_io.hpp" OUTPUT_VARIABLE ParserHpp)
add_custom_target(parsers COMMENT "Generating types from README.md"
    COMMAND edicta -i ${CMAKE_CURRENT_LIST_DIR}/README.md -o pspecs ${Programs}
    COMMAND ${CMAKE_CURRENT_LIST_DIR}/iogen -i pspecs
    BYPRODUCTS ${ParserHpp})

add_executable(writeimage src/writeimage.cpp ${Sources})
setup_main_program(writeimage)

add_executable(readimage src/readimage.cpp ${Sources})
setup_main_program(readimage)

install(TARGETS ${Programs} RUNTIME DESTINATION bin)

set(Profilers readfloatarray readfloatarray2 readstringarray readintarray)
add_custom_target(profile COMMENT "Building programs to be run with profiler")
add_dependencies(profile ${Profilers})
set(JSONProfiler common/BlockQueue.cpp common/InputChannel.cpp common/FileDescriptorInput.cpp common/JSONParsers.cpp)

# Custom target that generates all profiler parsers at the same time.
list(TRANSFORM Profilers APPEND "_io.hpp" OUTPUT_VARIABLE GenHpp)
add_custom_target(generated
    COMMENT "Generating profiler types from profile/profile.md"
    COMMAND edicta -i ${CMAKE_CURRENT_LIST_DIR}/profile/profile.md -o specs ${Profilers}
    COMMAND ${CMAKE_CURRENT_LIST_DIR}/iogen -i specs
    BYPRODUCTS ${GenHpp})

add_executable(readfloatarray EXCLUDE_FROM_ALL profile/readparse.cpp ${JSONProfiler})
setup_profiling(readfloatarray)

add_executable(readfloatarray2 EXCLUDE_FROM_ALL profile/readparse.cpp ${JSONProfiler})
setup_profiling(readfloatarray2)

add_executable(readstringarray EXCLUDE_FROM_ALL profile/readparse.cpp ${JSONProfiler})
setup_profiling(readstringarray)

add_executable(readintarray EXCLUDE_FROM_ALL profile/readparse.cpp ${JSONProfiler})
setup_profiling(readintarray)
