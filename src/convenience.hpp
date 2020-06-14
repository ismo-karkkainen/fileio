//
//  convenience.hpp
//
//  Created by Ismo Kärkkäinen on 13.6.2020.
//  Copyright © 2020 Ismo Kärkkäinen. All rights reserved.
//
// Licensed under Universal Permissive License. See License.txt.

// Helpers to deal with color map and gettting interpolated values.

#if !defined(CONVENIENCE_HPP)
#define CONVENIENCE_HPP

#include <vector>
#include <exception>
#include <unistd.h>
#include <cerrno>
#include <iostream>


template<typename Pool, typename Parser, typename Result>
class InputParser {
private:
    bool eof;
    int fd;
    const size_t block_size = 65536;
    std::vector<char> buffer;
    Pool pp;
    Parser parser;

public:
    typedef int (*Worker)(Result& Val);

    InputParser(int FileDescriptor)
        : eof(false), fd(FileDescriptor), buffer(block_size + 1, 0) { }

    int ReadAndParse(Worker W) {
        const char* end = nullptr;
        while (!eof) {
            if (end == nullptr) {
                if (buffer.size() != block_size + 1)
                    buffer.resize(block_size + 1);
                errno = 0;
                int count = read(fd, &buffer.front(), block_size);
                eof = (count == 0) ||
                    (count < 0 && !(errno == EAGAIN || errno == EINTR));
                if (count <= 0)
                    continue;
                buffer.resize(count + 1);
                buffer.back() = 0;
                end = &buffer.front();
            }
            if (parser.Finished()) {
                end = pp.skipWhitespace(end, &buffer.back());
                if (end == nullptr)
                    continue;
            }
            try {
                end = parser.Parse(end, &buffer.back(), pp);
            }
            catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
                return 1;
            }
            if (!parser.Finished()) {
                end = nullptr;
                continue;
            }
            Result val;
            parser.Swap(val.values);
            int rv = W(val);
            if (rv)
                return rv;
        }
        return 0;
    }
};

#endif
