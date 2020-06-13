//
//  split2planes.cpp
//
//  Created by Ismo Kärkkäinen on 3.6.2020.
//  Copyright © 2020 Ismo Kärkkäinen. All rights reserved.
//
// Licensed under Universal Permissive License. See License.txt.

#include "split2planes_io.hpp"
using namespace io; // ThreadedReadParse does not know of the namespace name.
#if defined(UNITTEST)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#else
#include "FileDescriptorInput.hpp"
#include "ThreadedReadParse_t.hpp"
#endif
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <cmath>
#include <fstream>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <deque>

static size_t plane_count(Split2PlanesIn::planesType& Planes) {
    size_t count = 0;
    for (auto& row : Planes)
        if (row.empty())
            continue;
        else if (count == 0)
            count = row.front().size();
        else if (row.front().size() != count)
            throw "Third dimension size varies.";
    return count;
}

static void separate(std::vector<std::vector<float>>& Out,
    Split2PlanesIn::planesType& Planes, size_t Index)
{
    Out.resize(Planes.size());
    for (size_t row_index = 0; row_index < Planes.size(); ++row_index) {
        std::vector<float>& row = Out[row_index];
        std::vector<std::vector<float>>& src = Planes[row_index];
        row.resize(src.size());
        for (size_t k = 0; k < src.size(); ++k)
            row[k] = src[k][Index];
    }
}

#if !defined(UNITTEST)
static bool split2planes(Split2PlanesIn& Val) {
    size_t count = 0;
    try {
        count = plane_count(Val.planes());
    }
    catch (const char* msg) {
        std::cerr << msg << std::endl;
        return false;
    }
    std::cout << '{';
    std::vector<std::vector<float>> plane;
    std::vector<char> buffer;
    for (size_t k = 0; k < count; ++k) {
        separate(plane, Val.planes(), k);
        std::cout << "\"plane" << k << "\":";
        Write(std::cout, plane, buffer);
        if (k + 1 < count)
            std::cout << ',';
    }
    std::cout << '}' << std::endl;
    return true;
}

int main(int argc, char** argv) {
    int f = 0;
    if (argc > 1)
        f = open(argv[1], O_RDONLY);
    FileDescriptorInput input(f);
    std::deque<std::shared_ptr<Split2PlanesIn>> vals;
    std::mutex vals_mutex;
    std::condition_variable output_added;
    ThreadedReadParse<Split2PlanesIn_Parser, Split2PlanesIn> reader(
        input, vals, vals_mutex, output_added);
    while (!reader.Finished() || !vals.empty()) {
        std::unique_lock<std::mutex> output_lock(vals_mutex);
        if (vals.empty()) {
            output_added.wait(output_lock);
            output_lock.unlock();
            continue; // If woken because quitting, loop condition breaks.
        }
        std::shared_ptr<Split2PlanesIn> v(vals.front());
        vals.pop_front();
        output_lock.unlock();
        if (!split2planes(*v))
            return 1;
    }
    if (f)
        close(f);
    return 0;
}

#else

TEST_CASE("plane_count") {
    SUBCASE("All same") {
        std::vector<std::vector<float>> row;
        row.push_back(std::vector<float> { 0.0f, 1.0f });
        row.push_back(std::vector<float> { 2.0f, 3.0f });
        io::Split2PlanesIn::planesType planes;
        planes.push_back(row);
        planes.push_back(row);
        REQUIRE(plane_count(planes) == 2);
    }
    SUBCASE("Empty row") {
        std::vector<std::vector<float>> row;
        row.push_back(std::vector<float> { 0.0f, 1.0f });
        row.push_back(std::vector<float> { 2.0f, 3.0f });
        io::Split2PlanesIn::planesType planes;
        planes.push_back(std::vector<std::vector<float>>());
        planes.push_back(row);
        REQUIRE(plane_count(planes) == 2);
    }
    SUBCASE("Mismatch") {
        std::vector<std::vector<float>> row;
        row.push_back(std::vector<float> { 0.0f, 1.0f });
        row.push_back(std::vector<float> { 2.0f, 3.0f });
        io::Split2PlanesIn::planesType planes;
        planes.push_back(std::vector<std::vector<float>>());
        planes.back().push_back(std::vector<float> { 0.0f, 1.0f, 2.0f });
        planes.push_back(row);
        REQUIRE_THROWS_AS(plane_count(planes), const char*);
    }
}

TEST_CASE("separate") {
    std::vector<std::vector<float>> out;
    SUBCASE("Only one") {
        std::vector<std::vector<float>> row;
        row.push_back(std::vector<float> { 0.0f });
        row.push_back(std::vector<float> { 2.0f });
        io::Split2PlanesIn::planesType planes;
        planes.push_back(row);
        planes.push_back(row);
        separate(out, planes, 0);
        REQUIRE(out.size() == planes.size());
        for (size_t r = 0; r < out.size(); ++r) {
            REQUIRE(out[r].size() == planes[r].size());
            for (size_t k = 0; k < out[r].size(); ++k)
                REQUIRE(out[r][k] == planes[r][k][0]);
        }
    }
    SUBCASE("Second, vary row length") {
        io::Split2PlanesIn::planesType planes;
        std::vector<std::vector<float>> row;
        planes.push_back(row);
        row.push_back(std::vector<float> { 0.0f, 1.0f });
        planes.push_back(row);
        row.push_back(std::vector<float> { 2.0f, 3.0f });
        planes.push_back(row);
        separate(out, planes, 1);
        REQUIRE(out.size() == planes.size());
        for (size_t r = 0; r < out.size(); ++r) {
            REQUIRE(out[r].size() == planes[r].size());
            for (size_t k = 0; k < out[r].size(); ++k)
                REQUIRE(out[r][k] == planes[r][k][1]);
        }
    }
}

#endif
