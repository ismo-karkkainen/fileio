cmake_minimum_required(VERSION 3.10)
project(imageio CXX)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "RELEASE")
        set(BuildOptions -Wall -O2)
    else()
        set(BuildOptions -Wall -O0)
    endif()
    set(AlwaysSmallOptions -Os)
    set(ProfilerOptions -fprofile-instr-generate)
    set(ProfilerLinkOptions -fprofile-instr-generate)
    set(CxxStd -std=c++17)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "RELEASE")
        set(BuildOptions -Wall -O2)
    else()
        set(BuildOptions -Wall -O0)
    endif()
    set(AlwaysSmallOptions -Os)
    set(ProfilerOptions -pg)
    set(ProfilerLinkOptions -pg)
    set(CxxStd -std=c++17)
#elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
#  # using Intel C++
#elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
#  # using Visual Studio C++
else()
    set(BuildOptions "")
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
target_compile_options(unittest PRIVATE ${BuildOptions})
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
function(setup_tiff TGTNAME)
    if (TIFF_FOUND)
        target_include_directories(${TGTNAME} SYSTEM PRIVATE ${TIFF_INCLUDE_DIR})
        target_link_libraries(${TGTNAME} PRIVATE ${TIFF_LIBRARY})
    else()
        target_compile_definitions(${TGTNAME} PRIVATE NO_TIFF)
    endif()
endfunction()

if (NOT DEFINED ENV{NO_PNG})
    find_package(PNG)
endif()
if (PNG_FOUND)
    message(STATUS "PNG supported.")
else()
    message(STATUS "No PNG support.")
endif()
function(setup_png TGTNAME)
    if (PNG_FOUND)
        target_include_directories(${TGTNAME} SYSTEM PRIVATE ${PNG_INCLUDE_DIR})
        target_link_libraries(${TGTNAME} PRIVATE ${PNG_LIBRARY})
    elseif (NOT DEFINED ENV{NO_PNG})
        target_compile_definitions(${TGTNAME} PRIVATE NO_LIBPNG)
    else()
        target_compile_definitions(${TGTNAME} PRIVATE NO_LIBPNG)
        target_compile_definitions(${TGTNAME} PRIVATE NO_PNG)
    endif()
endfunction()


set(Programs readimage writeimage)

list(TRANSFORM Programs APPEND "_io.hpp" OUTPUT_VARIABLE ParserHpp)
add_custom_target(parsers COMMENT "Generating types from README.md"
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/README.md
    COMMAND edicta -i ${CMAKE_CURRENT_LIST_DIR}/README.md -o pspecs ${Programs}
    COMMAND ${CMAKE_CURRENT_LIST_DIR}/iogen -i pspecs
    BYPRODUCTS ${ParserHpp})

function(setup_main_program TGTNAME MAIN)
    add_executable(${TGTNAME} ${MAIN} ${Sources})
    add_dependencies(${TGTNAME} parsers)
    target_include_directories(${TGTNAME} SYSTEM PRIVATE /usr/local/include)
    target_include_directories(${TGTNAME} PRIVATE common)
    target_include_directories(${TGTNAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    setup_tiff(${TGTNAME})
    setup_png(${TGTNAME})
    target_compile_options(${TGTNAME} PRIVATE ${CxxStd})
    target_compile_options(${TGTNAME} PRIVATE ${BuildOptions})
    if (UNIX AND NOT APPLE)
        target_link_libraries(${TGTNAME} PRIVATE Threads::Threads)
    endif()
endfunction()

setup_main_program(readimage src/readimage.cpp)
setup_main_program(writeimage src/writeimage.cpp)

install(TARGETS ${Programs} RUNTIME DESTINATION bin)


# Programs for profiling the code.

set(Profilers readfloatarray readfloatarray2 readstringarray readintarray)
add_custom_target(profile COMMENT "Building programs to be run with profiler")
add_dependencies(profile ${Profilers})
set(Readers common/BlockQueue.cpp common/InputChannel.cpp common/FileDescriptorInput.cpp common/JSONParsers.cpp)

# Custom target that generates all profiler parsers at the same time.
list(TRANSFORM Profilers APPEND "_io.hpp" OUTPUT_VARIABLE GenHpp)
add_custom_target(generated
    COMMENT "Generating profiler types from profile/profile.md"
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/profile/profile.md
    COMMAND edicta -i ${CMAKE_CURRENT_LIST_DIR}/profile/profile.md -o specs ${Profilers}
    COMMAND ${CMAKE_CURRENT_LIST_DIR}/iogen -i specs
    BYPRODUCTS ${GenHpp})

function(setup_profiling TGTNAME)
    add_executable(${TGTNAME} EXCLUDE_FROM_ALL profile/readparse.cpp ${Readers})
    add_dependencies(${TGTNAME} generated)
    target_compile_definitions(${TGTNAME} PRIVATE HEADER=${TGTNAME}_io.hpp)
    target_include_directories(${TGTNAME} SYSTEM PRIVATE /usr/local/include)
    target_include_directories(${TGTNAME} PRIVATE common)
    target_include_directories(${TGTNAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_compile_options(${TGTNAME} PRIVATE ${CxxStd})
    target_compile_options(${TGTNAME} PRIVATE ${BuildOptions} ${ProfilerOptions})
    target_link_libraries(${TGTNAME} PRIVATE ${ProfilerLinkOptions})
endfunction()

setup_profiling(readfloatarray)
setup_profiling(readfloatarray2)
setup_profiling(readstringarray)
setup_profiling(readintarray)
