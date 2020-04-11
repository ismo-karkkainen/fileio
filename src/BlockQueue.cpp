//
//  BlockQueue.cpp
//  imageio
//
//  Created by Ismo Kärkkäinen on 10.2.2020.
//  Copyright © 2020 Ismo Kärkkäinen. All rights reserved.
//
// Licensed under Universal Permissive License. See License.txt.

#include "BlockQueue.hpp"


BlockQueue::BlockPtr BlockQueue::dequeue(
    std::unique_lock<std::mutex>& lock, bool wait)
{
    if (!queue.empty()) {
        BlockPtr tmp(queue.back());
        queue.pop_back();
        return tmp;
    }
    if (!wait || ended)
        return BlockPtr();
    waiter.wait(lock);
    return dequeue(lock, false);
}

BlockQueue::~BlockQueue() {
    // Any thread waiting at this point has dequeue access destructed object.
    waiter.notify_all();
}

BlockQueue::BlockPtr BlockQueue::Add(BlockPtr& Filled) {
    std::unique_lock<std::mutex> lock(mutex);
    queue.push_front(Filled);
    if (available) {
        BlockPtr tmp(available);
        available.reset();
        lock.unlock();
        waiter.notify_one();
        return tmp;
    }
    lock.unlock();
    waiter.notify_one();
    return BlockPtr(new Block());
}

BlockQueue::BlockPtr BlockQueue::Remove(
    BlockQueue::BlockPtr& Emptied, bool WaitForBlock)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (Emptied && !available) {
        available = Emptied;
        available->resize(available->capacity() - 1);
    }
    return dequeue(lock, WaitForBlock);
}

BlockQueue::BlockPtr BlockQueue::Remove(bool WaitForBlock) {
    std::unique_lock<std::mutex> lock(mutex);
    return dequeue(lock, WaitForBlock);
}

void BlockQueue::End() {
    ended = true;
    waiter.notify_all();
}

bool BlockQueue::Ended() const {
    if (!ended)
        return false;
    return Empty();
}

bool BlockQueue::Empty() const {
    std::lock_guard<std::mutex> lock(mutex);
    return queue.empty();
}

size_t BlockQueue::Size() const {
    std::lock_guard<std::mutex> lock(mutex);
    return queue.size();
}
