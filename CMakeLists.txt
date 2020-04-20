cmake_minimum_required(VERSION 3.12)
project(imageio CXX)

#### Parameter set-up and library support detection.

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
else()
    set(BuildOptions "")
endif()

if (UNIX AND NOT APPLE)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

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
    else()
        target_compile_definitions(${TGTNAME} PRIVATE NO_PNG)
    endif()
endfunction()


#### Main programs

set(Programs readimage writeimage)

set(Sources src/BlockQueue.cpp src/InputChannel.cpp src/FileDescriptorInput.cpp src/JSONParsers.cpp src/JSONWriters.cpp)

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
    target_include_directories(${TGTNAME} PRIVATE src)
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


#### Tests

enable_testing()

add_executable(unittest src/unittest.cpp ${Sources})
target_include_directories(unittest SYSTEM PRIVATE /usr/local/include)
target_include_directories(unittest PRIVATE src)
target_compile_definitions(unittest PRIVATE TESTDOC_UNITTEST)
target_compile_options(unittest PRIVATE ${CxxStd})
target_compile_options(unittest PRIVATE ${BuildOptions})
if (UNIX AND NOT APPLE)
    target_link_libraries(unittest PRIVATE Threads::Threads)
endif()
add_test(NAME UnitTest COMMAND unittest)

function(add_test_prog PROG)
    add_executable(${PROG} IMPORTED)
    set_property(TARGET ${PROG} PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/test/${PROG})
endfunction()

function(new_test TEST_NAME PROG WIDTH HEIGHT PLANES BITS FORMAT)
    add_test(NAME ${TEST_NAME} COMMAND ${PROG} ${WIDTH} ${HEIGHT} ${PLANES} ${BITS} ${FORMAT} $<TARGET_FILE:readimage> $<TARGET_FILE:writeimage>)
    set_property(TEST ${TEST_NAME} PROPERTY ENVIRONMENT "PATH=${CMAKE_CURRENT_LIST_DIR}:${CMAKE_CURRENT_LIST_DIR}/test:$ENV{PATH}")
endfunction()

add_test_prog(rwimage.sh)
new_test(p3ppm8 rwimage.sh 255 134 3 8 P3-ppm)
new_test(p3ppm16 rwimage.sh 317 251 3 16 P3-PPM)
new_test(p3ppm4 rwimage.sh 128 65 3 4 p3-PPM)
new_test(p3ppm10 rwimage.sh 127 63 3 10 p3-PPM)
new_test(ppm8 rwimage.sh 76 32 3 8 PPM)
new_test(ppm16 rwimage.sh 316 577 3 16 P6-PPM)
if (TIFF_FOUND)
    new_test(tiff1.8 rwimage.sh 271 98 1 8 tif)
    new_test(tiff2.8 rwimage.sh 421 312 2 8 tIf)
    new_test(tiff3.8 rwimage.sh 142 83 3 8 TIf)
    new_test(tiff3.16 rwimage.sh 98 66 3 16 TiFF)
    new_test(tiff4.8 rwimage.sh 256 256 4 8 TiF)
    new_test(tiff4.16 rwimage.sh 512 512 4 16 TiFf)
    new_test(tiff5.8 rwimage.sh 185 412 5 8 TiF)
    new_test(tiff5.16 rwimage.sh 73 92 5 16 TiFf)
endif()
if (PNG_FOUND)
    new_test(png1.8 rwimage.sh 271 98 1 8 PNG)
    new_test(png1.16 rwimage.sh 185 192 1 16 png)
    new_test(png2.8 rwimage.sh 421 312 2 8 Png)
    new_test(png2.16 rwimage.sh 171 326 2 16 pNG)
    new_test(png3.8 rwimage.sh 142 83 3 8 PnG)
    new_test(png3.16 rwimage.sh 98 66 3 16 pNg)
    new_test(png4.8 rwimage.sh 256 256 4 8 PNg)
    new_test(png4.16 rwimage.sh 512 512 4 16 pnG)
endif()


#### Programs for profiling the code.

set(Profilers readfloatarray readfloatarray2 readstringarray readintarray)
add_custom_target(profile COMMENT "Building programs to be run with profiler")
add_dependencies(profile ${Profilers})
set(Readers src/BlockQueue.cpp src/InputChannel.cpp src/FileDescriptorInput.cpp src/JSONParsers.cpp)

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
    target_include_directories(${TGTNAME} PRIVATE src)
    target_include_directories(${TGTNAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_compile_options(${TGTNAME} PRIVATE ${CxxStd})
    target_compile_options(${TGTNAME} PRIVATE ${BuildOptions} ${ProfilerOptions})
    target_link_libraries(${TGTNAME} PRIVATE ${ProfilerLinkOptions})
endfunction()

setup_profiling(readfloatarray)
setup_profiling(readfloatarray2)
setup_profiling(readstringarray)
setup_profiling(readintarray)
