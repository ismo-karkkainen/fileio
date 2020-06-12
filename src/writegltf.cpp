//
//  writegltf.cpp
//
//  Created by Ismo Kärkkäinen on 12.6.2020.
//  Copyright © 2020 Ismo Kärkkäinen. All rights reserved.
//
// Licensed under Universal Permissive License. See License.txt.

#include "writegltf_io.hpp"
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


static void base64encode(std::vector<char>& Out, const char* Src, size_t Len) {
    const char c[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    Out.resize(0);
    Out.reserve(4 * ((Len + 2) / 3));
    while (3 <= Len) {
        Out.push_back(c[(Src[0] >> 2) & 0x3f]);
        Out.push_back(c[((Src[0] & 0x3) << 4) | ((Src[1] >> 4) & 0xf)]);
        Out.push_back(c[((Src[1] & 0xf) << 2) | ((Src[2] >> 6) & 0x3)]);
        Out.push_back(c[Src[2] & 0x3f]);
        Src += 3;
        Len -= 3;
    }
    if (Len == 2) {
        Out.push_back(c[(Src[0] >> 2) & 0x3f]);
        Out.push_back(c[((Src[0] & 0x3) << 4) | ((Src[1] >> 4) & 0xf)]);
        Out.push_back(c[(Src[1] & 0xf) << 2]);
        Out.push_back('=');
    } else if (Len == 1) {
        Out.push_back(c[(Src[0] >> 2) & 0x3f]);
        Out.push_back(c[(Src[0] & 0x3) << 4]);
        Out.push_back('=');
        Out.push_back('=');
    }
}

#if !defined(UNITTEST)
static bool writegltf(WriteglTFIn& Val) {
    std::ofstream out(Val.filename().c_str());
    if (out.fail()) {
        std::cerr << "Failed to open: " << Val.filename() << std::endl;
        return false;
    }
    out << R"GLTF({"scenes":[{"nodes":[0]}],"nodes":[{"mesh":0}],
"meshes":[{"primitives":[{"attributes":{"POSITION":1)GLTF";
    if (Val.colorsGiven())
        out << R"GLTF(,"COLOR_0":2)GLTF";
    out << R"GLTF(},"indices":0}]}],)GLTF";
    // Convert all tri-strips (and later fans) to triangles.
    std::vector<std::uint32_t> tris;
    for (auto& strip : Val.tristrips())
        for (size_t k = 0; k < strip.size() - 2; ++k) {
            tris.push_back(strip[k]);
            if (k & 1) {
                tris.push_back(strip[k + 2]);
                tris.push_back(strip[k + 1]);
            } else {
                tris.push_back(strip[k + 1]);
                tris.push_back(strip[k + 2]);
            }
        }
    std::vector<char> buffer;
    size_t index_len = tris.size() * sizeof(std::uint32_t);
    base64encode(buffer, reinterpret_cast<const char*>(&(tris.front())),
        index_len);
    out << R"GLTF("buffers":[{"uri":"data:application/octet-stream;base64,)GLTF"
        << std::string(buffer.begin(), buffer.end())
        << R"GLTF(","byteLength":)GLTF" << index_len << "},\n";
    std::vector<float> flat;
    std::vector<float> vertex_max, vertex_min;
    vertex_max = vertex_min = Val.vertices().front();
    for (auto& vertex : Val.vertices())
        for (size_t k = 0; k < 3; ++k) {
            flat.push_back(vertex[k]);
            if (vertex_max[k] < vertex[k])
                vertex_max[k] = vertex[k];
            else if (vertex[k] < vertex_min[k])
                vertex_min[k] = vertex[k];
        }
    size_t vertex_len = flat.size() * sizeof(float);
    base64encode(buffer, reinterpret_cast<const char*>(&(flat.front())),
        vertex_len);
    out << R"GLTF({"uri":"data:application/octet-stream;base64,)GLTF"
        << std::string(buffer.begin(), buffer.end())
        << R"GLTF(","byteLength":)GLTF" << vertex_len << "}";
    size_t color_len = 0;
    std::vector<float> color_max, color_min;
    if (Val.colorsGiven()) {
        flat.resize(0);
        color_max = color_min = Val.colors().front();
        for (auto& color : Val.colors())
            for (auto& component : color) {
                flat.push_back(component);
                if (color_max[k] < color[k])
                    color_max[k] = color[k];
                else if (color[k] < color_min[k])
                    color_min[k] = color[k];
            }
        color_len = flat.size() * sizeof(float);
        base64encode(buffer, reinterpret_cast<const char*>(&(flat.front())),
            color_len);
        out << ",\n"
            << R"GLTF({"uri":"data:application/octet-stream;base64,)GLTF"
            << std::string(buffer.begin(), buffer.end())
            << R"GLTF(","byteLength":)GLTF" << color_len << "}";
    }
    out << R"GLTF(],
"bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":)GLTF"
        << index_len << R"GLTF(,"target":34963},
{"buffer":1,"byteOffset":0,"byteLength":)GLTF"
        << vertex_len << R"GLTF(,"target":34962})GLTF";
    if (Val.colorsGiven())
        out << R"GLTF(,
{"buffer":2,"byteOffset":0,"byteLength":)GLTF"
            << color_len << R"GLTF(,"target":34962})GLTF";
    out << R"GLTF(],
"accessors":[{"bufferView":0,"byteOffset":0,"componentType":5125,"count":)GLTF"
        << (index_len / sizeof(std::uint32_t))
        << R"GLTF(,"type":"SCALAR","max":[)GLTF"
        << vertex_len / (sizeof(float) * 3) - 1
        << R"GLTF(],"min":[0]},
{"bufferView":1,"byteOffset":0,"componentType":5126,"count":)GLTF"
        << (vertex_len / (sizeof(float) * 3))
        << R"GLTF(,"type":"VEC3","max":[)GLTF"
        << vertex_max[0] << ',' << vertex_max[1] << ',' << vertex_max[2]
        << R"GLTF(],"min":[)GLTF"
        << vertex_min[0] << ',' << vertex_min[1] << ',' << vertex_min[2]
        << "]}";
    if (Val.colorsGiven())
        out << R"GLTF(,
{"bufferView":2,"byteOffset":0,"componentType":5126,"count":)GLTF"
            << (color_len / (sizeof(float) * 3))
            << R"GLTF(,"type":"VEC3","max":[)GLTF"
            << color_max[0] << ',' << color_max[1] << ',' << color_max[2]
            << R"GLTF(],"min":[)GLTF"
            << color_min[0] << ',' << color_min[1] << ',' << color_min[2]
            << "]}";
    out << R"GLTF(],
"asset":{"version":"2.0"}})GLTF";
    bool ok = out.good();
    out.close();
    return ok;
}

int main(int argc, char** argv) {
    int f = 0;
    if (argc > 1)
        f = open(argv[1], O_RDONLY);
    FileDescriptorInput input(f);
    std::deque<std::shared_ptr<WriteglTFIn>> vals;
    std::mutex vals_mutex;
    std::condition_variable output_added;
    ThreadedReadParse<WriteglTFIn_Parser, WriteglTFIn> reader(
        input, vals, vals_mutex, output_added);
    while (!reader.Finished() || !vals.empty()) {
        std::unique_lock<std::mutex> output_lock(vals_mutex);
        if (vals.empty()) {
            output_added.wait(output_lock);
            output_lock.unlock();
            continue; // If woken because quitting, loop condition breaks.
        }
        std::shared_ptr<WriteglTFIn> v(vals.front());
        vals.pop_front();
        output_lock.unlock();
        if (!writegltf(*v))
            return 1;
    }
    if (f)
        close(f);
    return 0;
}

#else

#endif
