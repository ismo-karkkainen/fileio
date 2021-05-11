//
//  writegltf.cpp
//
//  Created by Ismo Kärkkäinen on 12.6.2020.
//  Copyright © 2020-2021 Ismo Kärkkäinen. All rights reserved.
//
// Licensed under Universal Permissive License. See License.txt.

#include "writegltf_io.hpp"
#if defined(UNITTEST)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#else
#include "convenience.hpp"
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
    const char c[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
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

static size_t flatten(std::vector<float>& Out,
    std::vector<float>& Min, std::vector<float>& Max,
    const std::vector<std::vector<float>>& Src)
{
    Out.resize(0);
    Out.reserve(Src.size() * 3);
    Min.resize(3);
    Max.resize(3);
    for (size_t k = 0; k < 3; ++k)
        Min[k] = Max[k] = Src.front()[k];
    for (auto& vertex : Src)
        for (size_t k = 0; k < 3; ++k) {
            Out.push_back(vertex[k]);
            if (Max[k] < vertex[k])
                Max[k] = vertex[k];
            else if (vertex[k] < Min[k])
                Min[k] = vertex[k];
        }
    return Out.size() * sizeof(float);
}

static void buffer_object(std::ofstream& Out,
    const std::vector<char>& Buffer, size_t Length)
{
    Out << R"GLTF({"uri":"data:application/octet-stream;base64,)GLTF"
        << std::string(Buffer.begin(), Buffer.end())
        << R"GLTF(","byteLength":)GLTF" << Length << "}";
}

static void accessor_object(std::ofstream& Out, size_t Count,
    const std::vector<float>& Min, const std::vector<float>& Max)
{
    Out << R"GLTF({"bufferView":1,"byteOffset":0,"componentType":5126,"count":)GLTF"
        << Count << R"GLTF(,"type":"VEC3","max":[)GLTF"
        << Max[0] << ',' << Max[1] << ',' << Max[2] << R"GLTF(],"min":[)GLTF"
        << Min[0] << ',' << Min[1] << ',' << Min[2] << "]}";
}

#if !defined(UNITTEST)
static int writegltf(io::WriteglTFIn& Val) {
    if (Val.filename().substr(Val.filename().size() - 5) != ".gltf")
        Val.filename() += ".gltf";
    std::ofstream out(Val.filename().c_str());
    if (out.fail()) {
        std::cerr << "Failed to open: " << Val.filename() << std::endl;
        return 1;
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
    out << R"GLTF("buffers":[)GLTF";
    buffer_object(out, buffer, index_len);
    std::vector<float> flat, vertex_max, vertex_min;
    size_t vertex_len = flatten(flat, vertex_min, vertex_max, Val.vertices());
    base64encode(buffer, reinterpret_cast<const char*>(&(flat.front())),
        vertex_len);
    out << ",\n";
    buffer_object(out, buffer, vertex_len);
    size_t color_len = 0;
    std::vector<float> color_max, color_min;
    if (Val.colorsGiven()) {
        color_len = flatten(flat, color_min, color_max, Val.colors());
        base64encode(buffer, reinterpret_cast<const char*>(&(flat.front())),
            color_len);
        out << ",\n";
        buffer_object(out, buffer, color_len);
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
        << R"GLTF(],"min":[0]},)GLTF" << "\n";
    accessor_object(out,
        vertex_len / (sizeof(float) * 3), vertex_min, vertex_max);
    if (Val.colorsGiven()) {
        out << ",\n";
        accessor_object(out,
            color_len / (sizeof(float) * 3), color_min, color_max);
    }
    out << R"GLTF(],
"asset":{"version":"2.0"}})GLTF";
    bool ok = out.good();
    out.close();
    return ok ? 0 : 2;
}

int main(int argc, char** argv) {
    int f = 0;
    if (argc > 1)
        f = open(argv[1], O_RDONLY);
    InputParser<io::ParserPool, io::WriteglTFIn_Parser, io:: WriteglTFIn> ip(f);
    int status = ip.ReadAndParse(writegltf);
    if (f)
        close(f);
    return status;
}

#else

#endif
