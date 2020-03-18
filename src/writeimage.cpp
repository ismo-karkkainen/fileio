#include "writeimage_io.hpp"
#include "FileDescriptorInput.hpp"
#include "ThreadedReadParse_t.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <tiffio.h>
#include <vector>
#include <cmath>


typedef int (*WriteFunc)(const WriteImageIOValues::filenameType&, const WriteImageIOValues::imageType&, WriteImageIOValues::depthType);

static int writeTIFF(const WriteImageIOValues::filenameType& filename,
    const WriteImageIOValues::imageType& image,
    WriteImageIOValues::depthType depth)
{
    TIFF* t = TIFFOpen(filename.c_str(), "w");
    if (!t) {
        std::cerr << "Failed to open output file: " << filename << std::endl;
        return 10;
    }
    TIFFSetField(t, TIFFTAG_IMAGEWIDTH, image[0].size());
    TIFFSetField(t, TIFFTAG_IMAGELENGTH, image.size());
    TIFFSetField(t, TIFFTAG_SAMPLESPERPIXEL, image[0][0].size());
    TIFFSetField(t, TIFFTAG_BITSPERSAMPLE, depth);
    TIFFSetField(t, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(t, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(t, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    uint32 count = 0;
    std::vector<unsigned char> d8;
    d8.reserve(image[0].size() * image[0][0].size());
    std::vector<unsigned short> d16;
    d16.reserve(image[0].size() * image[0][0].size());
    for (auto& line : image) {
        tdata_t data = nullptr;
        if (depth == 8) {
            d8.resize(0);
            for (auto& pixel : line)
                for (auto& component : pixel)
                    d8.push_back(
                        static_cast<unsigned char>(round(255 * component)));
            data = static_cast<tdata_t>(&d8.front());
        } else if (depth == 16) {
            d16.resize(0);
            for (auto& pixel : line)
                for (auto& component : pixel)
                    d16.push_back(
                        static_cast<unsigned short>(round(65535 * component)));
            data = static_cast<tdata_t>(&d16.front());
        }
        if (TIFFWriteScanline(t, data, count++, 0) != 1) {
            TIFFClose(t);
            std::cerr << "Error writing to output: " << filename << std::endl;
            unlink(filename.c_str());
            return 11;
        }
    }
    TIFFClose(t);
    return 0;
}

static int writePNG(const WriteImageIOValues::filenameType& filename,
    const WriteImageIOValues::imageType& image,
    WriteImageIOValues::depthType depth)
{
    return 0;
}

int main(int argc, char** argv) {
    int f = 0;
    if (argc > 1)
        f = open(argv[1], O_RDONLY);
    FileDescriptorInput input(f);
    WriteImageIOValues val;
    try {
        ThreadedReadParse<WriteImageIO, WriteImageIOValues> reader(input, val);
        if (f)
            close(f);
        if (!reader.Finished())
            return 1;
    }
    catch (const ParserException& e) {
        std::cerr << e.what() << std::endl;
        return 2;
    }
    if (val.image().empty()) {
        std::cerr << "Image has zero height." << std::endl;
        return 3;
    }
    if (val.image().empty() || val.image()[0].empty()) {
        std::cerr << "Image has zero width." << std::endl;
        return 3;
    }
    if (val.image()[0][0].size() != 3) {
        std::cerr << "Got " << val.image()[0][0].size() <<
            " color planes, not 3." << std::endl;
        return 3;
    }
    // Check type presence. If not given, use file name extension.
    if (!val.formatGiven()) {
        size_t last = val.filename().find_last_of(".");
        if (last == std::string::npos) {
            std::cerr << "No format nor extension in filename." << std::endl;
            return 4;
        }
        val.format() = val.filename().substr(last + 1);
    }
    WriteFunc writer = nullptr;
    if (strcasecmp(val.format().c_str(), "tiff") == 0 ||
        strcasecmp(val.format().c_str(), "tif") == 0)
    {
        // TIFF-writer.
        writer = &writeTIFF;
        if (8 < val.depth())
            val.depth() = 16;
        else if (val.depth() <= 8)
            val.depth() = 8;
    } else if (strcasecmp(val.format().c_str(), "png") == 0) {
        // PNG-writer.
        writer = &writePNG;
        if (8 < val.depth())
            val.depth() = 16;
        else if (val.depth() <= 8)
            val.depth() = 8;
    } else {
        std::cerr << "Unsupported format: " << val.format() << std::endl;
        return 4;
    }
    // Find minimum and maximum, if at least one is missing.
    if (!val.minimumGiven() || !val.maximumGiven())
        for (auto& line : val.image())
            for (auto& pixel : line)
                for (auto& component : pixel) {
                    if (!val.minimumGiven() && component < val.minimum())
                        val.minimum() = component;
                    if (!val.maximumGiven() && val.maximum() < component)
                        val.maximum() = component;
                }
    // Limit values using minimum and maximum.
    float range = val.maximum() - val.minimum();
    if (range < 0) {
        std::cerr << "Maximum (" << val.maximum() << ") < minimum ("
            << val.minimum() << ")." << std::endl;
        return 5;
    }
    for (auto& line : val.image()) {
        if (line.size() != val.image()[0].size()) {
            std::cerr << "Image width not constant, " << line.size() << " != "
                << val.image()[0].size() << std::endl;
            return 6;
        }
        for (auto& pixel : line) {
            if (pixel.size() != val.image()[0][0].size()) {
                std::cerr << "Color depth not constant, " << pixel.size() <<
                    " != " << val.image()[0][0].size() << std::endl;
                return 6;
            }
            for (auto& component : pixel) {
                component -= val.minimum();
                if (component <= 0.0f)
                    component = 0.0f;
                else if (range <= component)
                    component = 1.0f;
                else
                    component /= range;
            }
        }
    }
    return writer(val.filename(), val.image(), val.depth());
}
