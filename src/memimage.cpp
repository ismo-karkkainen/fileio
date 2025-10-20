//
//  memimage.cpp
//
//  Created by Ismo Kärkkäinen on 25.6.2020.
//  Copyright © 2020 Ismo Kärkkäinen. All rights reserved.
//
// Licensed under Universal Permissive License. See License.txt.

#include "memimage.hpp"
#include <cmath>
#include <cinttypes>
#if !defined(NO_PNG)
#include <cstring>
#include <memory>
#include <csetjmp>
#include <png.h>
#endif


template<typename T>
class Buffer : public std::vector<T> {
public:
    Buffer& operator<<(T c) {
        this->push_back(c);
        return *this;
    }

    Buffer& write_u32(std::uint32_t s) {
        this->push_back((s >> 24) & 0xff);
        this->push_back((s >> 16) & 0xff);
        this->push_back((s >> 8) & 0xff);
        this->push_back(s & 0xff);
        return *this;
    }
};

#if !defined(NO_PNG)
static void png_error_handler(png_structp unused, const char* error) {
    throw error;
}

static void png_warning_handler(png_structp unused, const char* unused2) { }

typedef void (*png_destroyer)(png_structp);
static void destroy_png(png_structp p) {
    png_destroy_write_struct(&p, nullptr);
}

typedef void (*info_destroyer)(png_infop);
static png_structp png_s = nullptr;
static void destroy_info(png_infop p) {
    png_destroy_info_struct(png_s, &p);
}

static void append(png_structp Png, png_bytep Data, size_t Length) {
    if (Length == 0)
        return;
    std::vector<unsigned char>* out =
        reinterpret_cast<std::vector<unsigned char>*>(png_get_io_ptr(Png));
    out->resize(out->size() + Length);
    memcpy(&(out->front()) + (out->size() - Length), Data, Length);
}

std::vector<unsigned char> memoryPNG(
    const std::vector<std::vector<std::vector<float>>>& Image, int Depth)
{
    std::vector<unsigned char> out;
    std::unique_ptr<png_struct,png_destroyer> png(
        png_create_write_struct(PNG_LIBPNG_VER_STRING,
            nullptr, &png_error_handler, &png_warning_handler),
        &destroy_png);
    if (!png)
        return out;
    png_set_write_fn(png.get(), reinterpret_cast<void*>(&out), append, nullptr);
    png_s = png.get();
    std::unique_ptr<png_info,info_destroyer> info(
        png_create_info_struct(png.get()), &destroy_info);
    if (!info)
        return out;
    if (setjmp(png_jmpbuf(png.get())))
        return out;
    int color_type;
    switch (Image[0][0].size()) {
    case 1: color_type = PNG_COLOR_TYPE_GRAY; break;
    case 2: color_type = PNG_COLOR_TYPE_GRAY_ALPHA; break;
    case 3: color_type = PNG_COLOR_TYPE_RGB; break;
    case 4: color_type = PNG_COLOR_TYPE_RGB_ALPHA; break;
    }
    png_set_IHDR(png.get(), info.get(), Image[0].size(), Image.size(), Depth,
        color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE);
    png_write_info(png.get(), info.get());
    std::vector<std::uint32_t> rows;
    rows.reserve(Image.size());
    const size_t row_size = Image[0].size() * Image[0][0].size() * (Depth / 8);
    Buffer<char> buf;
    buf.reserve(Image.size() * row_size);
    for (auto& line : Image) {
        for (auto& pixel : line)
            if (Depth == 8)
                for (auto& component : pixel)
                    buf << static_cast<char>(
                        static_cast<unsigned char>(component));
            else
                for (auto& component : pixel) {
                    std::uint16_t val = static_cast<std::uint16_t>(component);
                    buf << static_cast<char>((val >> 8) & 0xff)
                        << static_cast<char>(val & 0xff);
                }
        rows.push_back(rows.empty() ? 0 : rows.back() + row_size);
    }
    std::vector<png_bytep> row_pointers;
    row_pointers.reserve(Image.size());
    for (const auto idx : rows) {
        row_pointers.push_back(reinterpret_cast<png_bytep>(&buf.front()) + idx);
    }
    png_write_image(png.get(), &row_pointers.front());
    png_write_end(png.get(), info.get());
    return out;
}

#endif
