#include "writeimage_io.hpp"
#include "FileDescriptorInput.hpp"
#include "ThreadedReadParse_t.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>


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
    // Check type presence. If not given, use file name extension.
    if (!val.formatGiven()) {
        size_t last = val.filenameGet().find_last_of(".");
        if (last == std::string::npos) {
            std::cerr << "No format not extension in filename." << std::endl;
            return 3;
        }
        val.formatGet() = val.filenameGet().substr(last);
    }
    if (strcasecmp(val.formatGet().c_str(), "tiff") == 0 ||
        strcasecmp(val.formatGet().c_str(), "tif") == 0)
    {
        // TIFF-writer.
        if (8 < val.depthGet())
            val.depthGet() = 16;
        else if (val.depthGet() <= 8)
            val.depthGet() = 8;
    } else if (strcasecmp(val.formatGet().c_str(), "png") == 0) {
        // PNG-writer.
        if (8 < val.depthGet())
            val.depthGet() = 16;
        else if (val.depthGet() <= 8)
            val.depthGet() = 8;
    }
    return 0;
}
