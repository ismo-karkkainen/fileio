//
//  writeimage.cpp
//
//  Created by Ismo Kärkkäinen on 17.3.2020.
//  Copyright © 2020 Ismo Kärkkäinen. All rights reserved.
//
// Licensed under Universal Permissive License. See License.txt.

#include "writeimage_io.hpp"
#include "convenience.hpp"
#include "memimage.hpp"
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
#if !defined(NO_TIFF)
#include <tiffio.h>
#endif
#if !defined(NO_PNG)
#include <memory>
#include <csetjmp>
#include <png.h>
#endif


typedef int (*WriteFunc)(const io::WriteImageIn::filenameType&, const io::WriteImageIn::imageType&, io::WriteImageIn::depthType);

#if !defined(NO_TIFF)

static int writeTIFF(const io::WriteImageIn::filenameType& filename,
    const io::WriteImageIn::imageType& image, io::WriteImageIn::depthType depth)
{
    TIFF* t = TIFFOpen(filename.c_str(), "w");
    if (!t) {
        std::cerr << "Failed to open output file: " << filename << std::endl;
        return 1;
    }
    TIFFSetField(t, TIFFTAG_IMAGEWIDTH,
        static_cast<std::uint32_t>(image[0].size()));
    TIFFSetField(t, TIFFTAG_IMAGELENGTH,
        static_cast<std::uint32_t>(image.size()));
    TIFFSetField(t, TIFFTAG_SAMPLESPERPIXEL,
        static_cast<std::uint16_t>(image[0][0].size()));
    TIFFSetField(t, TIFFTAG_BITSPERSAMPLE, static_cast<std::uint16_t>(depth));
    TIFFSetField(t, TIFFTAG_MAXSAMPLEVALUE,
        static_cast<std::uint16_t>((1 << depth) - 1));
    TIFFSetField(t, TIFFTAG_MINSAMPLEVALUE, 0);
    TIFFSetField(t, TIFFTAG_COMPRESSION, static_cast<std::uint16_t>(1));
    TIFFSetField(t, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(t, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    if (image[0][0].size() < 3) {
        TIFFSetField(t, TIFFTAG_PHOTOMETRIC, static_cast<std::uint16_t>(1));
        if (image[0][0].size() == 2) {
            std::uint16_t other(2);
            TIFFSetField(t, TIFFTAG_EXTRASAMPLES,
                static_cast<std::uint16_t>(1), &other);
        }
    } else {
        TIFFSetField(t, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
        if (image[0][0].size() > 3) {
            // Guess that the first is unassociated alpha and the rest unknown.
            std::vector<std::uint16_t> other;
            other.push_back(2);
            for (size_t k = 4; k < image[0][0].size(); ++k)
                other.push_back(0);
            TIFFSetField(t, TIFFTAG_EXTRASAMPLES,
                static_cast<std::uint16_t>(other.size()), &other.front());
        }
    }
    uint32 count = 0;
    std::vector<unsigned char> buf;
    buf.reserve(image[0].size() * image[0][0].size() * ((8 < depth) ? 2 : 1));
    for (auto& line : image) {
        buf.resize(0);
        for (auto& pixel : line)
            if (depth == 8)
                for (auto& component : pixel)
                    buf.push_back(static_cast<unsigned char>(component));
            else
                for (auto& component : pixel) {
                    buf.push_back(0);
                    buf.push_back(0);
                    *reinterpret_cast<std::uint16_t*>((&buf.back()) - 1) =
                        static_cast<std::uint16_t>(component);
                }
        if (TIFFWriteScanline(t, static_cast<tdata_t>(&buf.front()), count++, 0) != 1)
        {
            TIFFClose(t);
            std::cerr << "Error writing to output: " << filename << std::endl;
            unlink(filename.c_str());
            return 2;
        }
    }
    TIFFClose(t);
    return 0;
}
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

static int write_png(const char* filename,
    const io::WriteImageIn::imageType& image, io::WriteImageIn::depthType depth)
{
    std::ofstream out;
    out.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    out.open(filename,
        std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
    std::vector<unsigned char> buf = memoryPNG(image, depth);
    if (buf.empty())
        return 1;
    out.write(reinterpret_cast<char*>(&buf.front()), buf.size());
    out.close();
    return 0;
}

static int writePNG(const io::WriteImageIn::filenameType& filename,
    const io::WriteImageIn::imageType& image, io::WriteImageIn::depthType depth)
{
    try {
        switch (write_png(filename.c_str(), image, depth)) {
        case 0: return 0;
        case 1:
            std::cerr << "Error creating PNG.\n";
            return 1;
        }
    }
    catch (std::ofstream::failure& e) {
        std::cerr << filename << ": " << e.what() << "\n";
        return 2;
    }
    catch (const char* e) {
        std::cerr << e << "\n";
        return 3;
    }
    std::cerr << "Unspecified error.\n";
    return 4;
}

#endif

// PPM, NetPBM color image binary format.

static int writePPM(const io::WriteImageIn::filenameType& filename,
    const io::WriteImageIn::imageType& image, io::WriteImageIn::depthType depth)
{
    std::ofstream out;
    out.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    out.open(filename,
        std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
    std::stringstream header;
    header << "P6\n" << image[0].size() << '\n' << image.size() << '\n'
        << ((1 << depth) - 1) << '\n';
    out << header.str();
    Buffer<char> buf;
    for (auto& line : image)
        for (auto& pixel : line)
            for (auto& component : pixel)
                if (depth == 8)
                    buf << static_cast<char>(static_cast<unsigned char>(
                        component));
                else {
                    std::uint16_t val = static_cast<std::uint16_t>(component);
                    buf << static_cast<char>((val >> 8) & 0xff)
                        << static_cast<char>(val & 0xff);
                }
    out.write(&buf.front(), buf.size());
    out.close();
    return 0;
}

// PPM, NetPBM color image text format.

static int writePlainPPM(const io::WriteImageIn::filenameType& filename,
    const io::WriteImageIn::imageType& image, io::WriteImageIn::depthType depth)
{
    std::ofstream out;
    out.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    out.open(filename, std::ofstream::out | std::ofstream::trunc);
    out << "P3\n" << image[0].size() << '\n' << image.size() << '\n'
        << (1 << depth) - 1 << '\n';
    for (auto& line : image)
        for (auto& pixel : line) // We know there are 3 components.
            out << pixel[0] << ' ' << pixel[1] << ' ' << pixel[2] << '\n';
    out.close();
    return 0;
}

static int write_image(io::WriteImageIn& val) {
    if (val.image().empty()) {
        std::cerr << "Image has zero height.\n";
        return 1;
    }
    if (val.image()[0].empty()) {
        std::cerr << "Image has zero width.\n";
        return 1;
    }
    if (val.image()[0][0].empty()) {
        std::cerr << "Image has zero depth.\n";
        return 1;
    }
    // Check type presence. If not given, use file name extension.
    if (!val.formatGiven()) {
        size_t last = val.filename().find_last_of(".");
        if (last == std::string::npos) {
            std::cerr << "No format nor extension in filename.\n";
            return 1;
        }
        val.format() = val.filename().substr(last + 1);
    }
    WriteFunc writer = nullptr;
#if !defined(NO_TIFF)
    bool tiff = false;
#endif
    if (strcasecmp(val.format().c_str(), "ppm") == 0 ||
        strcasecmp(val.format().c_str(), "p6-ppm") == 0)
    {
        // PPM-writer.
        writer = &writePPM;
        if (8 < val.depth())
            val.depth() = 16;
        else if (val.depth() <= 8)
            val.depth() = 8;
        if (val.image()[0][0].size() != 3) {
            std::cerr << "Got " << val.image()[0][0].size() <<
                " color planes, not 3.\n";
            return 1;
        }
    } else if (strcasecmp(val.format().c_str(), "p3-ppm") == 0) {
        // Plain text-format PPM writer.
        writer = &writePlainPPM;
        if (val.depth() < 1)
            val.depth() = 1;
        else if (16 < val.depth())
            val.depth() = 16;
        if (val.image()[0][0].size() != 3) {
            std::cerr << "Got " << val.image()[0][0].size() <<
                " color planes, not 3.\n";
            return 1;
        }
#if !defined(NO_TIFF)
    } else if (strcasecmp(val.format().c_str(), "tiff") == 0 ||
        strcasecmp(val.format().c_str(), "tif") == 0)
    {
        // TIFF-writer.
        tiff = true;
        writer = &writeTIFF;
        if (8 < val.depth())
            val.depth() = 16;
        else if (val.depth() <= 8)
            val.depth() = 8;
#endif
#if !defined(NO_PNG)
    } else if (strcasecmp(val.format().c_str(), "png") == 0) {
        // PNG-writer.
        writer = &writePNG;
        if (8 < val.depth())
            val.depth() = 16;
        else if (val.depth() <= 8)
            val.depth() = 8;
        if (4 < val.image()[0][0].size()) {
            std::cerr << "Too many color planes: " <<
                val.image()[0][0].size() << std::endl;
            return 1;
        }
#endif
    } else {
        std::cerr << "Unsupported format: " << val.format() << std::endl;
        return 1;
    }
    // Find minimum and maximum, if at least one is missing.
    if (!val.minimumGiven() || !val.maximumGiven()) {
        if (!val.minimumGiven())
            val.minimum() = val.image()[0][0][0];
        if (!val.maximumGiven())
            val.maximum() = val.image()[0][0][0];
        for (auto& line : val.image())
            for (auto& pixel : line)
                for (auto& component : pixel) {
                    if (!val.minimumGiven() && component < val.minimum())
                        val.minimum() = component;
                    if (!val.maximumGiven() && val.maximum() < component)
                        val.maximum() = component;
                }
    }
    // Limit values using minimum and maximum.
    float range = val.maximum() - val.minimum();
    if (range < 0) {
        std::cerr << "Maximum (" << val.maximum() << ") < minimum ("
            << val.minimum() << ").\n";
        return 1;
    }
    for (auto& line : val.image()) {
        if (line.front().size() != val.image()[0][0].size()) {
            std::cerr << "Color component count not constant, " <<
                line.front().size() << " != " << val.image()[0][0].size() << "\n";
            return 1;
        }
        for (auto& pixel : line) {
            for (auto& component : pixel) {
                component -= val.minimum();
                if (component <= 0.0f)
                    component = 0.0f;
                else if (range <= component)
                    component = 1.0f;
                else {
                    component /= range;
                    if (1.0f < component)
                        component = 1.0f;
                }
            }
        }
    }
#if !defined(NO_TIFF)
    if (tiff && val.image()[0][0].size() < 3)
        val.depth() = 8; // Grayscale TIFF does not support 16-bit depth.
#endif
    // Scale the components here since depth is known.
    float max = 1 << val.depth();
    for (auto& line : val.image())
        for (auto& pixel : line)
            for (auto& component : pixel) {
                component = trunc(component * max);
                if (component == max)
                    component = max - 1;
            }
    try {
        writer(val.filename(), val.image(), val.depth());
    }
    catch (std::ofstream::failure f) {
        unlink(val.filename().c_str());
        std::cerr << f.code() << ' ' << f.what() << '\n';
        return 2;
    }
    return 0;
}

int main(int argc, char** argv) {
    int f = 0;
    if (argc > 1)
        f = open(argv[1], O_RDONLY);
    InputParser<io::ParserPool, io::WriteImageIn_Parser, io::WriteImageIn>
        ip(f);
    int status = ip.ReadAndParse(write_image);
    if (f)
        close(f);
    return status;
}
