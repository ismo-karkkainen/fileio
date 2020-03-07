//
//  InputChannel.hpp
//  imageio / datalackey
//
//  Created by Ismo Kärkkäinen on 10.5.17.
//  Copyright © 2017, 2020 Ismo Kärkkäinen. All rights reserved.
//
// Licensed under Universal Permissive License. See License.txt.

#ifndef InputChannel_hpp
#define InputChannel_hpp

#include <cstddef>


// Base class for input channels.
class InputChannel {
public:
    virtual ~InputChannel();
    virtual int Read(char* Buffer, size_t Length) = 0;
    // Return true if the channel has closed.
    virtual bool Ended() = 0;
};


#endif /* InputChannel_hpp */
