#include "FileDescriptorInput.hpp"
#include "BlockQueue.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <vector>
#include <cmath>
#include <fstream>
#include <cstddef>
#include <iterator>
#include <cstdint>
#if !defined(NO_TIFF)
#include <tiffio.h>
#endif

typedef std::vector<std::vector<std::vector<float>>> Image;
#define READIMAGEOUT_TYPE ReadImageOutTemplate<Image,std::string>

#include "readimage_io.hpp"


int read_whole_file(std::vector<std::byte>& Contents, const char* Filename) {
    int fd = open(Filename, O_RDONLY | O_CLOEXEC);
    if (fd == -1)
        return -1;
    struct stat info;
    if (-1 == fstat(fd, &info)) {
        close(fd);
        return -2;
    }
    Contents.resize(info.st_size);
    int got = read(fd, &Contents.front(), info.st_size);
    close(fd);
    return Contents.size() - got;
}


typedef const char* (*ReadFunc)(const ReadImageInValues::filenameType&, Image&);

#if !defined(NO_TIFF)
static int read_tiff(
    const ReadImageInValues::filenameType& filename, Image& image)
{
    TIFFSetWarningHandler(NULL);
    TIFF* t = TIFFOpen(filename.c_str(), "r");
    if (t == nullptr)
        return -1;
    uint16 bits, samples;
    uint32 width, height;
    TIFFGetField(t, TIFFTAG_BITSPERSAMPLE, &bits);
    if (bits != 8 && bits != 16) {
        TIFFClose(t);
        return -2;
    }
    TIFFGetField(t, TIFFTAG_SAMPLESPERPIXEL, &samples);
    TIFFGetField(t, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(t, TIFFTAG_IMAGELENGTH, &height);
    uint32 config;
    TIFFGetField(t, TIFFTAG_PLANARCONFIG, &config);
    if (config != PLANARCONFIG_CONTIG) {
        TIFFClose(t);
        return -3;
    }
    tsize_t line_size = TIFFScanlineSize(t);
    tdata_t buffer = _TIFFmalloc(line_size);
    image.resize(height);
    uint32 row = 0;
    for (auto& line : image) {
        line.resize(width);
        TIFFReadScanline(t, buffer, row++);
        unsigned char* curr = reinterpret_cast<unsigned char*>(buffer);
        for (auto& pixel : line) {
            pixel.resize(samples);
            for (auto& component : pixel)
                if (bits == 8)
                    component = float(*curr++);
                else {
                    component = float(*curr) * 256.0f + float(curr[1]);
                    curr += 2;
                }
        }
    }
    _TIFFfree(buffer);
    TIFFClose(t);
    return 0;
}

static const char* readTIFF(
    const ReadImageInValues::filenameType& filename, Image& image)
{
    int status = read_tiff(filename, image);
    if (status == 0)
        return nullptr;
    switch (status) {
    case -1: return "Failed to open file.";
    case -2: return "Unsupported bit depth.";
    case -3: return "Not contiguous planar configuration.";
    }
    return "Unspecified error.";
}
#endif

// PPM, NetPBM color image binary format.

static int read_ppm(
    const ReadImageInValues::filenameType& filename, Image& image)
{
    std::vector<std::byte> contents;
    int status = read_whole_file(contents, filename.c_str());
    if (status != 0)
        return status;
    // Read P6 width height maximum
    if (contents.size() < 12)
        return -3;
    if (contents[0] != static_cast<std::byte>('P') && contents[1] != static_cast<std::byte>('6'))
        return -3;
    int width, height, maxval;
    size_t idx;
    try {
        ParserPool pp;
        ParseInt p;
        const char* last = reinterpret_cast<const char*>(&contents.back());
        const char* curr = p.skipWhitespace(
            reinterpret_cast<const char*>(&contents.front() + 2),
            reinterpret_cast<const char*>(&contents.back()));
        curr = p.Scan(curr, last, pp);
        if (curr == nullptr || !p.isWhitespace(*curr))
            return -4;
        width = std::get<ParserPool::Int>(pp.Value);
        curr = p.skipWhitespace(curr, last);
        curr = p.Scan(curr, last, pp);
        if (curr == nullptr || !p.isWhitespace(*curr))
            return -4;
        height = std::get<ParserPool::Int>(pp.Value);
        curr = p.skipWhitespace(curr, last);
        curr = p.Scan(curr, last, pp);
        if (curr == nullptr || !p.isWhitespace(*curr))
            return -4;
        maxval = std::get<ParserPool::Int>(pp.Value);
        if (width <= 0 || height <= 0 || maxval <= 0 || 65535 < maxval)
            return -4;
        ++curr;
        idx = reinterpret_cast<const std::byte*>(++curr) - &contents.front();
        if (contents.size() - idx + 1 != width * height * ((maxval < 256) ? 3 : 6))
            return -5;
    }
    catch (const ParserException& e) {
        return -4;
    }
    image.resize(height);
    for (auto& line : image) {
        line.resize(width);
        for (auto& pixel : line) {
            pixel.resize(3);
            for (auto& component : pixel) {
                if (maxval < 256) {
                    component = float(contents[idx]);
                    ++idx;
                } else {
                    component = float(contents[idx]) * 256 + float(contents[idx + 1]);
                    idx += 2;
                }
            }
        }
    }
    return 0;
}

static const char* readPPM(
    const ReadImageInValues::filenameType& filename, Image& image)
{
    int status = read_ppm(filename, image);
    if (status > 0)
        return "Failed to read whole file.";
    if (status == 0)
        return nullptr;
    switch (status) {
    case -1: return "Failed to open file.";
    case -2: return "Failed to get file size.";
    case -3: return "Not PPM.";
    case -4: return "Invalid header.";
    case -5: return "File and header size mismatch.";
    }
    return "Unspecified error.";
}

static void report(const ReadImageOut& Out) {
    std::vector<char> buffer;
    Write(std::cout, Out, buffer);
}

const size_t block_size = 4096; // Presumably way bigger than single input.

int main(int argc, char** argv) {
    int f = 0;
    if (argc > 1)
        f = open(argv[1], O_RDONLY);
    FileDescriptorInput input(f);
    BlockQueue::BlockPtr buffer;
    ParserPool pp;
    ReadImageIn parser;
    const char* end = nullptr;
    while (!input.Ended()) {
        if (end == nullptr) {
            if (!buffer)
                buffer.reset(new BlockQueue::Block());
            if (buffer->size() != block_size + 1)
                buffer->resize(block_size + 1);
            int count = input.Read(&buffer->front(), block_size);
            if (count == 0)
                continue;
            buffer->resize(count + 1);
            buffer->back() = 0;
            end = &buffer->front();
        }
        if (parser.Finished()) {
            end = pp.skipWhitespace(end, &buffer->back());
            if (end == nullptr)
                continue;
        }
        try {
            end = parser.Scan(end, &buffer->back(), pp);
        }
        catch (const ParserException& e) {
            std::cerr << e.what() << std::endl;
            return 1;
        }
        if (!parser.Finished()) {
            end = nullptr;
            continue;
        }
        ReadImageInValues val;
        parser.Swap(val.values);
        ReadImageOut out;
        if (!val.formatGiven()) {
            size_t last = val.filename().find_last_of(".");
            if (last == std::string::npos) {
                out.error = "No format nor extension in filename.";
                report(out);
                continue;
            }
            val.format() = val.filename().substr(last + 1);
        }
        ReadFunc reader = nullptr;
        float shift = 0.0f;
        float scale = 1.0f;
        if (val.minimumGiven()) {
            shift = val.minimum();
            if (val.maximumGiven()) {
                if (val.maximum() <= val.minimum()) {
                    out.error = "maximum <= minimum";
                    report(out);
                    continue;
                }
                scale = val.maximum() - val.minimum();
            }
        } else if (val.maximumGiven())
            shift = val.maximum();
        if (strcasecmp(val.format().c_str(), "ppm") == 0)
            reader = &readPPM;
#if !defined(NO_TIFF)
        else if (strcasecmp(val.format().c_str(), "tiff") == 0 ||
            strcasecmp(val.format().c_str(), "tif") == 0)
                reader = &readTIFF;
#endif
        //else if (strcasecmp(val.format().c_str(), "png") == 0)
            //reader = &readPNG;
        else {
            out.error = "Unsupported format: " + val.format();
            report(out);
            continue;
        }
        const char* err = reader(val.filename(), out.image);
        if (err) {
            out.error = std::string(err);
            report(out);
            continue;
        }
        float minval, maxval;
        minval = maxval = out.image[0][0][0];
        for (auto& line : out.image)
            for (auto& pixel : line)
                for (auto& component : pixel) {
                    if (component < minval)
                        minval = component;
                    if (maxval < component)
                        maxval = component;
                }
        if (val.minimumGiven() || val.maximumGiven())
            shift += -minval;
        if (minval == maxval)
            scale = 1.0f;
        else if (val.minimumGiven() && val.maximumGiven())
            scale /= (maxval - minval);
        for (auto& line : out.image)
            for (auto& pixel : line)
                for (auto& component : pixel)
                    component = (component + shift) * scale;
        report(out);
    }
    return 0;
}
