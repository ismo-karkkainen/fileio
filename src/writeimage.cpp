#include "writeimage_io.hpp"
using namespace writeio;
#include "FileDescriptorInput.hpp"
#include "ThreadedReadParse_t.hpp"
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


typedef int (*WriteFunc)(const WriteImageIn::filenameType&, const WriteImageIn::imageType&, WriteImageIn::depthType);

#if !defined(NO_TIFF)

static int writeTIFF(const WriteImageIn::filenameType& filename,
    const WriteImageIn::imageType& image, WriteImageIn::depthType depth)
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
            for (auto& component : pixel)
                if (depth == 8)
                    buf.push_back(static_cast<unsigned char>(component));
                else {
                    std::uint16_t val = static_cast<std::uint16_t>(component);
                    buf.push_back((val >> 8) & 0xFF);
                    buf.push_back(val & 0xFF);
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
static void png_error_handler(png_structp unused, const char* error) {
    throw error;
}

static void png_warning_handler(png_structp unused, const char* unused2) { }

static const char* png_filename;
static void clear_file(FILE* file) {
    if (file != nullptr) {
        fclose(file);
        if (png_filename)
            unlink(png_filename);
    }
}

typedef void (*png_destroyer)(png_structp);
static void destroy_png(png_structp p) {
    png_destroy_write_struct(&p, nullptr);
}

typedef void (*info_destroyer)(png_infop);
static png_structp png_s = nullptr;
static void destroy_info(png_infop p) {
    png_destroy_info_struct(png_s, &p);
}

static std::string png_error_message;


static int write_png(const char* filename, const WriteImageIn::imageType& image,
    WriteImageIn::depthType depth)
{
    png_filename = filename;
    std::unique_ptr<FILE,void (*)(FILE*)> file(
        fopen(filename, "wb"), &clear_file);
    if (!file)
        return 1;
    std::unique_ptr<png_struct,png_destroyer> png(
        png_create_write_struct(PNG_LIBPNG_VER_STRING,
            nullptr, &png_error_handler, &png_warning_handler),
        &destroy_png);
    if (!png)
        return 3;
    png_s = png.get();
    std::unique_ptr<png_info,info_destroyer> info(
        png_create_info_struct(png.get()), &destroy_info);
    if (!info)
        return 3;
    if (setjmp(png_jmpbuf(png.get())))
        return 2;
    png_init_io(png.get(), file.get());
    int color_type;
    switch (image[0][0].size()) {
    case 1: color_type = PNG_COLOR_TYPE_GRAY; break;
    case 2: color_type = PNG_COLOR_TYPE_GRAY_ALPHA; break;
    case 3: color_type = PNG_COLOR_TYPE_RGB; break;
    case 4: color_type = PNG_COLOR_TYPE_RGB_ALPHA; break;
    }
    png_set_IHDR(png.get(), info.get(), image[0].size(), image.size(), depth,
        color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE);
    png_write_info(png.get(), info.get());
    std::vector<png_bytep> row_pointers;
    row_pointers.reserve(image.size());
    const size_t row_size = image[0].size() * image[0][0].size() * (depth / 8);
    Buffer<char> buf;
    buf.reserve(image.size() * row_size);
    for (auto& line : image) {
        for (auto& pixel : line)
            for (auto& component : pixel)
                if (depth == 8)
                    buf << static_cast<char>(
                        static_cast<unsigned char>(component));
                else {
                    std::uint16_t val = static_cast<std::uint16_t>(component);
                    buf << static_cast<char>((val >> 8) & 0xff)
                        << static_cast<char>(val & 0xff);
                }
        if (!row_pointers.empty())
            row_pointers.push_back(row_pointers.back() + row_size);
        else
            row_pointers.push_back(reinterpret_cast<png_bytep>(&buf.front()));
    }
    png_write_image(png.get(), &row_pointers.front());
    png_write_end(png.get(), info.get());
    png_filename = nullptr;
    return 0;
}

static int writePNG(const WriteImageIn::filenameType& filename,
    const WriteImageIn::imageType& image, WriteImageIn::depthType depth)
{
    try {
        switch (write_png(filename.c_str(), image, depth)) {
        case 0: return 0;
        case 1:
            std::cerr << "Failed to open: " << filename << "\n";
            return 1;
        case 2:
            std::cerr << "Failed to write to file: " << filename << "\n";
            return 2;
        case 3:
            std::cerr << "Internal error.\n";
            return 3;
        }
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

static int writePPM(const WriteImageIn::filenameType& filename,
    const WriteImageIn::imageType& image, WriteImageIn::depthType depth)
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

static int writePlainPPM(const WriteImageIn::filenameType& filename,
    const WriteImageIn::imageType& image, WriteImageIn::depthType depth)
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

static void write_image(WriteImageIn& val) {
    if (val.image().empty()) {
        std::cerr << "Image has zero height.\n";
        return;
    }
    if (val.image()[0].empty()) {
        std::cerr << "Image has zero width.\n";
        return;
    }
    if (val.image()[0][0].empty()) {
        std::cerr << "Image has zero depth.\n";
        return;
    }
    // Check type presence. If not given, use file name extension.
    if (!val.formatGiven()) {
        size_t last = val.filename().find_last_of(".");
        if (last == std::string::npos) {
            std::cerr << "No format nor extension in filename.\n";
            return;
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
            return;
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
            return;
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
            return;
        }
#endif
    } else {
        std::cerr << "Unsupported format: " << val.format() << std::endl;
        return;
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
        return;
    }
    for (auto& line : val.image()) {
        if (line.size() != val.image()[0].size()) {
            std::cerr << "Image width not constant, " << line.size() << " != "
                << val.image()[0].size() << std::endl;
            return;
        }
        for (auto& pixel : line) {
            if (pixel.size() != val.image()[0][0].size()) {
                std::cerr << "Color component count not constant, " <<
                    pixel.size() << " != " << val.image()[0][0].size() << "\n";
                return;
            }
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
    }
}

int main(int argc, char** argv) {
    int f = 0;
    if (argc > 1)
        f = open(argv[1], O_RDONLY);
    FileDescriptorInput input(f);
    std::deque<std::shared_ptr<WriteImageIn>> vals;
    std::mutex vals_mutex;
    std::condition_variable output_added;
    ThreadedReadParse<WriteImageIn_Parser, WriteImageIn> reader(
        input, vals, vals_mutex, output_added);
    while (!reader.Finished() || !vals.empty()) {
        std::unique_lock<std::mutex> output_lock(vals_mutex);
        if (vals.empty()) {
            output_added.wait(output_lock);
            output_lock.unlock();
            continue; // If woken because quitting, loop condition breaks.
        }
        std::shared_ptr<WriteImageIn> v(vals.front());
        vals.pop_front();
        output_lock.unlock();
        write_image(*v);
    }
    if (f)
        close(f);
    return 0;
}
