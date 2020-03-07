//
//  BlockQueue.cpp
//  imageio
//
//  Created by Ismo Kärkkäinen on 10.2.2020.
//  Copyright © 2020 Ismo Kärkkäinen. All rights reserved.
//
// Licensed under Universal Permissive License. See License.txt.

#include "BlockQueue.hpp"


BlockQueue::BlockPtr BlockQueue::dequeue() {
    if (!queue.empty()) {
        BlockPtr tmp(queue.back());
        queue.pop_back();
        return tmp;
    }
    return BlockPtr();
}

BlockQueue::BlockPtr BlockQueue::Add(BlockPtr& Filled) {
    std::lock_guard<std::mutex> lock(mutex);
    queue.push_front(Filled);
    if (available) {
        BlockPtr tmp(available);
        available.reset();
        return tmp;
    }
    return BlockPtr(new Block());
}

BlockQueue::BlockPtr BlockQueue::Remove(BlockQueue::BlockPtr& Emptied) {
    std::lock_guard<std::mutex> lock(mutex);
    if (Emptied && !available) {
        available = Emptied;
        available->resize(available->capacity() - 1);
    }
    return dequeue();
}

BlockQueue::BlockPtr BlockQueue::Remove() {
    std::lock_guard<std::mutex> lock(mutex);
    return dequeue();
}
