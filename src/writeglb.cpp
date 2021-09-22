//
//  writeglb.cpp
//
//  Created by Ismo Kärkkäinen on 22.6.2020.
//  Copyright © 2020 Ismo Kärkkäinen. All rights reserved.
//
// Licensed under Universal Permissive License. See License.txt.

#include "writeglb_io.hpp"
#if defined(UNITTEST)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#else
#include "convenience.hpp"
#endif
#include "memimage.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <cmath>
#include <fstream>
#include <cstddef>
#include <cstdint>
#include <strstream>
#include <deque>


template<typename T>
class Buffer : public std::vector<T> {
public:
    Buffer& operator<<(T c) {
        this->push_back(c);
        return *this;
    }

    Buffer& operator<<(int c) {
        this->push_back(static_cast<T>(c));
        return *this;
    }

    Buffer& operator<<(const T* c) {
        while (*c)
            this->push_back(*c++);
        return *this;
    }

    Buffer& write_u32(std::uint32_t s) {
        this->push_back(s & 0xff);
        this->push_back((s >> 8) & 0xff);
        this->push_back((s >> 16) & 0xff);
        this->push_back((s >> 24) & 0xff);
        return *this;
    }

    Buffer& write_u32(std::uint32_t s, size_t index) {
        (*this)[index] = s & 0xff;
        (*this)[++index] = (s >> 8) & 0xff;
        (*this)[++index] = (s >> 16) & 0xff;
        (*this)[++index] = (s >> 24) & 0xff;
        return *this;
    }
};

static size_t flatten(std::vector<float>& Out,
    std::vector<float>& Min, std::vector<float>& Max,
    const std::vector<std::vector<float>>& Src)
{
    Out.resize(0);
    Out.reserve(Src.size() * Src.front().size());
    Min.resize(Src.front().size());
    Max.resize(Src.front().size());
    for (size_t k = 0; k < Src.front().size(); ++k)
        Min[k] = Max[k] = Src.front()[k];
    for (auto& vertex : Src)
        for (size_t k = 0; k < Src.front().size(); ++k) {
            Out.push_back(vertex[k]);
            if (Max[k] < vertex[k])
                Max[k] = vertex[k];
            else if (vertex[k] < Min[k])
                Min[k] = vertex[k];
        }
    return Out.size() * sizeof(float);
}

#if !defined(UNITTEST)
static int writeglb(io::WriteGLBIn& Val) {
    if (Val.filename().substr(Val.filename().size() - 4) != ".glb")
        Val.filename() += ".glb";
    Buffer<char> header, json_chunk, bin;
    header.write_u32(0x46546C67).write_u32(2);
    json_chunk.write_u32(0).write_u32(0x4E4F534A);
    std::strstream json;
    bin.write_u32(0).write_u32(0x004E4942);
    json << R"GLTF({"scenes":[{"nodes":[0]}],"nodes":[{"mesh":0}],
"meshes":[{"primitives":[{"attributes":{"POSITION":1)GLTF";
    if (Val.coordinatesGiven())
        json << R"GLTF(,"TEXCOORD_0":2)GLTF";
    json << R"GLTF(},"indices":0,"mode":4)GLTF";
    if (Val.textureGiven())
        json << R"GLTF(,"material":0)GLTF";
    json << R"GLTF(})GLTF";
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
    for (auto& idx : tris)
        bin.write_u32(idx);
    size_t index_len = bin.size() - 8;
    size_t end_of_previous = index_len;
    std::vector<float> flat, vertex_max, vertex_min;
    size_t vertex_len = flatten(flat, vertex_min, vertex_max, Val.vertices());
    for (auto& v : flat)
        bin.write_u32(*reinterpret_cast<std::uint32_t*>(&v));
    json << R"GLTF(]}],
"bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":)GLTF"
        << index_len << R"GLTF(,"target":34963},
{"buffer":0,"byteOffset":)GLTF"
        << end_of_previous << R"GLTF(,"byteLength":)GLTF"
        << vertex_len << R"GLTF(,"target":34962})GLTF";
    end_of_previous += vertex_len;
    size_t coordinates_len = 0;
    std::vector<float> coordinates_max, coordinates_min;
    if (Val.coordinatesGiven()) {
        coordinates_len = flatten(flat,
            coordinates_min, coordinates_max, Val.coordinates());
        for (auto& v : flat)
            bin.write_u32(*reinterpret_cast<std::uint32_t*>(&v));
        json << R"GLTF(,
{"buffer":0,"byteOffset":)GLTF"
            << end_of_previous << R"GLTF(,"byteLength":)GLTF"
            << coordinates_len << R"GLTF(,"target":34962})GLTF";
    }
    size_t image_len = 0;
    int image_max = 0;
    if (Val.textureGiven()) {
        std::vector<unsigned char> img = memoryPNG(Val.texture(), 8);
        image_len = img.size();
        for (auto& b : img) {
            if (image_max < b)
                image_max = b;
            bin << static_cast<char>(b);
        }
        json << R"GLTF(,
{"buffer":0,"byteOffset":)GLTF"
            << end_of_previous + coordinates_len << R"GLTF(,"byteLength":)GLTF"
            << image_len << R"GLTF(})GLTF";
    }
    json << R"GLTF(],
"accessors":[{"bufferView":0,"byteOffset":0,"componentType":5125,"count":)GLTF"
        << (index_len / sizeof(std::uint32_t))
        << R"GLTF(,"type":"SCALAR","max":[)GLTF"
        << vertex_len / (sizeof(float) * 3) - 1
        << R"GLTF(],"min":[0]},)GLTF" << "\n";
    json << R"GLTF({"bufferView":1,"byteOffset":0,"componentType":5126,"count":)GLTF"
        << vertex_len / (sizeof(float) * 3)
        << R"GLTF(,"type":"VEC3","max":[)GLTF"
        << vertex_max[0] << ',' << vertex_max[1] << ',' << vertex_max[2]
        << R"GLTF(],"min":[)GLTF"
        << vertex_min[0] << ',' << vertex_min[1] << ',' << vertex_min[2] << "]}";
    if (Val.coordinatesGiven()) {
        json << R"GLTF(,{"bufferView":2,"byteOffset":0,"componentType":5126,"count":)GLTF"
            << coordinates_len / (sizeof(float) * 2)
            << R"GLTF(,"type":"VEC2","max":[)GLTF"
            << coordinates_max[0] << ',' << coordinates_max[1]
            << R"GLTF(],"min":[)GLTF"
            << coordinates_min[0] << ',' << coordinates_min[1] << "]}";
        end_of_previous += coordinates_len;
    }
    if (Val.textureGiven())
        json << R"GLTF(,{"bufferView":3,"byteOffset":0,"componentType":5121,"count":)GLTF"
            << image_len << R"GLTF(,"type":"SCALAR","max":[)GLTF"
            << image_max << R"GLTF(],"min":[0]}],
"textures":[{"sampler":0,"source":0}],
"images":[{"bufferView":3,"mimeType":"image/png"}],
"samplers":[{"magFilter":9729,"minFilter":9729,"wrapS":33071,"wrapT":33071}],
"materials":[{"pbrMetallicRoughness":{"baseColorTexture":{"index":0},"metallicFactor":0.0}}
)GLTF";
    json << R"GLTF(],"buffers":[{"byteLength":)GLTF"
        << bin.size() - 8 << R"GLTF(}],"asset":{"version":"2.0"}})GLTF"
        << std::ends;
    json_chunk << json.str();
    json.freeze(false);
    while (json_chunk.size() & 0x3)
        json_chunk << ' ';
    json_chunk.write_u32(json_chunk.size() - 8, 0);
    while (bin.size() & 0x3)
        bin << '\0';
    bin.write_u32(bin.size() - 8, 0);
    header.write_u32(header.size() + 4 + json_chunk.size() + bin.size());
    std::ofstream out(Val.filename().c_str(),
        std::ios_base::out | std::ios_base::binary);
    if (out.fail()) {
        std::cerr << "Failed to open: " << Val.filename() << std::endl;
        return 1;
    }
    out.write(&header.front(), header.size());
    out.write(&json_chunk.front(), json_chunk.size());
    out.write(&bin.front(), bin.size());
    bool ok = out.good();
    out.close();
    return ok ? 0 : 2;
}

int main(int argc, char** argv) {
    int f = 0;
    if (argc > 1)
        f = open(argv[1], O_RDONLY);
    InputParser<io::ParserPool, io::WriteGLBIn_Parser, io::WriteGLBIn> ip(f);
    int status = ip.ReadAndParse(writeglb);
    if (f)
        close(f);
    return status;
}

#else

#endif
