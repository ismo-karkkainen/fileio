#include "writeimage_io.hpp"
#include "FileDescriptorInput.hpp"
#include "ThreadedReadParse_t.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <cmath>
#include <limits>
#include <fstream>
#include <cstddef>
#include <iterator>
#include <cstdint>
#include <sstream>
#include <deque>
#if !defined(NO_TIFF)
#include <tiffio.h>
#endif


typedef int (*WriteFunc)(const WriteImageInValues::filenameType&, const WriteImageInValues::imageType&, WriteImageInValues::depthType);

#if !defined(NO_TIFF)

static int writeTIFF(const WriteImageInValues::filenameType& filename,
    const WriteImageInValues::imageType& image,
    WriteImageInValues::depthType depth)
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

static void add_adler32(Buffer<char>& buf, size_t start) {
    uint32_t s1 = 1; // Or use 16-bit values?
    uint32_t s2 = 0;
    for (; start < buf.size(); ++start) {
        s1 = (s1 + buf[start]) % 65521;
        s2 = (s2 + s1) % 65521;
    }
    buf << ((s2 >> 8) & 0xff) << (s2 & 0xff)
        << ((s1 >> 8) & 0xff) << (s1 & 0xff);
}

// https://www.w3.org/TR/PNG/#D-CRCAppendix
static std::uint32_t crc_table[256];
static void make_crc_table() {
    for (std::uint32_t n = 0; n < 256; ++n) {
        std::uint32_t c = n;
        for (int k = 0; k < 8; ++k)
            c = (c & 1) ? 0xedb88320L ^ (c >> 1) : c >> 1;
        crc_table[n] = c;
    }
}

static void add_crc(Buffer<char>& buf) {
    unsigned char* b = reinterpret_cast<unsigned char*>((&buf.front()) + 4);
    unsigned char* e = b + buf.size() - 4;
    std::uint32_t c = 0xffFFffFFL;
    for (; b != e; ++b)
        c = crc_table[(c ^ *b) & 0xff] ^ (c >> 8);
    c = c ^ 0xffFFffFFL;
    buf.write_u32(c);
}

static int writePNG(const WriteImageInValues::filenameType& filename,
    const WriteImageInValues::imageType& image,
    WriteImageInValues::depthType depth)
{
    // https://stackoverflow.com/questions/7942635/write-png-quickly
    make_crc_table();
    std::ofstream out;
    out.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    out.open(filename,
        std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
    // https://www.w3.org/TR/PNG/#11Chunks
    // Signature.
    unsigned char sig[] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    out.write(reinterpret_cast<char*>(sig), 8);
    Buffer<char> buf;
    // IHDR
    buf << 0 << 0 << 0 << 0
        << 73 << 72 << 68 << 82;
    buf.write_u32(image[0].size()).write_u32(image.size())
        << (depth & 0xff);
    switch (image[0][0].size()) {
    case 1: buf << 0; break;
    case 2: buf << 4; break;
    case 3: buf << 2; break;
    case 4: buf << 6; break;
    }
    buf << 0 << 0 << 0;
    buf[3] = buf.size() - 8;
    add_crc(buf);
    out.write(&buf.front(), buf.size());
    // All lines, 0 for filter per line, then pixels.
    decltype(buf) filtered;
    for (auto& line : image) {
        filtered << 0; // No filtering.
        for (auto& pixel : line)
            for (auto& component : pixel)
                if (depth == 8)
                    filtered << static_cast<char>(
                        static_cast<unsigned char>(component));
                else {
                    std::uint16_t val = static_cast<std::uint16_t>(component);
                    filtered << static_cast<char>((val >> 8) & 0xff)
                        << static_cast<char>(val & 0xff);
                }
    }
    buf.resize(0);
    size_t header = 0;
    size_t sz = 0;
    for (size_t k = 0; k < filtered.size(); ++k) {
        if (buf.size() == 0) {
            buf << 0 << 0 << 0 << 0;
            buf << 73 << 68 << 65 << 84;
            if (k == 0) {
                buf << 8; // No compression.
                buf << 0x1d;
            }
            size_t remain = filtered.size() - k;
            if (remain < 65536) {
                // Size and its complement are least-significant byte first.
                buf << 1 << (remain & 0xff) << ((remain >> 8) & 0xff);
                buf << ~buf[buf.size() - 2] << ~buf[buf.size() - 2];
                sz = remain;
            } else {
                buf << 0 << 0xff << 0xff << 0 << 0;
                sz = 65535;
            }
            header = buf.size();
            sz += header;
        }
        buf << filtered[k];
        if (buf.size() == sz) {
            if (buf[header - 5])
                add_adler32(buf, header - 5);
            buf[0] = ((buf.size() - 8) >> 24) & 0xff;
            buf[1] = ((buf.size() - 8) >> 16) & 0xff;
            buf[2] = ((buf.size() - 8) >> 8) & 0xff;
            buf[3] = (buf.size() - 8) & 0xff;
            add_crc(buf);
            out.write(&buf.front(), buf.size());
            buf.resize(0);
        }
    }
    buf << 0 << 0 << 0 << 0;
    buf << 73 << 69 << 78 << 68;
    add_crc(buf);
    out.write(&buf.front(), buf.size());
    out.close();
    return 0;
}

#endif

// PPM, NetPBM color image binary format.

static int writePPM(const WriteImageInValues::filenameType& filename,
    const WriteImageInValues::imageType& image,
    WriteImageInValues::depthType depth)
{
    std::ofstream out;
    out.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    out.open(filename,
        std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
    std::stringstream header;
    header << "P6\n" << image[0].size() << '\n' << image.size() << '\n'
        << ((depth == 8) ? "255" : "65535") << '\n';
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

static int writePlainPPM(const WriteImageInValues::filenameType& filename,
    const WriteImageInValues::imageType& image,
    WriteImageInValues::depthType depth)
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

static void write_image(WriteImageInValues& val) {
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
    // Shift and scale values to [0, 1[ range.
    float maxval = 1.0f - std::numeric_limits<float>::epsilon();
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
                    component = maxval;
                else {
                    component /= range;
                    if (maxval < component)
                        component = maxval;
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
            for (auto& component : pixel)
                component = trunc(component * max);
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
    std::deque<std::shared_ptr<WriteImageInValues>> vals;
    std::mutex vals_mutex;
    std::condition_variable output_added;
    ThreadedReadParse<WriteImageIn, WriteImageInValues> reader(
        input, vals, vals_mutex, output_added);
    while (!reader.Finished() || !vals.empty()) {
        std::unique_lock<std::mutex> output_lock(vals_mutex);
        if (vals.empty()) {
            output_added.wait(output_lock);
            output_lock.unlock();
            continue; // If woken because quitting, loop condition breaks.
        }
        std::shared_ptr<WriteImageInValues> v(vals.front());
        vals.pop_front();
        output_lock.unlock();
        write_image(*v);
    }
    if (f)
        close(f);
    return 0;
}
