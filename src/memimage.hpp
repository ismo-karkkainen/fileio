//
//  memimage.hpp
//
//  Created by Ismo Kärkkäinen on 25.6.2020.
//  Copyright © 2020 Ismo Kärkkäinen. All rights reserved.
//
// Licensed under Universal Permissive License. See License.txt.

#if !defined(MEMIMAGE_HPP)
#define MEMIMAGE_HPP

#include <vector>


#if !defined(NO_PNG)
std::vector<unsigned char> memoryPNG(
    const std::vector<std::vector<std::vector<float>>>& Image, int Depth);
#endif

#endif
