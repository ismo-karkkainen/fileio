//
//  FileDescriptorInput.hpp
//  imageio / datalackey
//
//  Created by Ismo Kärkkäinen on 24.5.17.
//  Copyright © 2017, 2020 Ismo Kärkkäinen. All rights reserved.
//
// Licensed under Universal Permissive License. See License.txt.

#ifndef FileDescriptorInput_hpp
#define FileDescriptorInput_hpp

#include "InputChannel.hpp"


class FileDescriptorInput : public InputChannel {
private:
    bool eof;
    int fd;

public:
    FileDescriptorInput(int FileDescriptor = 0);
    ~FileDescriptorInput();
    int Read(char* Buffer, size_t Length);
    bool Ended();
};


#endif /* FileDescriptorInput_hpp */
