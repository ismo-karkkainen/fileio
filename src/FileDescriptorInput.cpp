//
//  FileDescriptorInput.cpp
//  imageio / datalackey
//
//  Created by Ismo Kärkkäinen on 24.5.17.
//  Copyright © 2017, 2020 Ismo Kärkkäinen. All rights reserved.
//
// Licensed under Universal Permissive License. See License.txt.

#include "FileDescriptorInput.hpp"
#include <sys/select.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <cerrno>


FileDescriptorInput::FileDescriptorInput(int FileDescriptor)
    : eof(false), fd(FileDescriptor)
{ }

FileDescriptorInput::~FileDescriptorInput() { }

int FileDescriptorInput::Read(char* Buffer, size_t Length) {
    struct timespec ts;
    fd_set std_in;
    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    FD_ZERO(&std_in);
    FD_SET(fd, &std_in);
    errno = 0;
    int avail = pselect(fd + 1, &std_in, nullptr, nullptr, &ts, nullptr);
    if (avail <= 0) {
        eof = avail < 0 && errno == EBADF;
        return 0;
    }
    errno = 0;
    int got = read(fd, Buffer, Length);
    if (got <= 0) {
        eof = got == 0 || (got < 0 && !(errno == EAGAIN || errno == EINTR));
        got = 0;
    }
    return got;
}

bool FileDescriptorInput::Ended() {
    return eof;
}
