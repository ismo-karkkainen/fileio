//
//  BlockQueue.hpp
//  imageio
//
//  Created by Ismo Kärkkäinen on 10.2.2020.
//  Copyright © 2020 Ismo Kärkkäinen. All rights reserved.
//
// Licensed under Universal Permissive License. See License.txt.

#ifndef BlockQueue_hpp
#define BlockQueue_hpp

#include <vector>
#include <deque>
#include <memory>
#include <mutex>
#if defined(__GNUG__)
#include <condition_variable>
#endif


class BlockQueue {
public:
    typedef std::vector<char> Block;
    typedef std::shared_ptr<Block> BlockPtr;

private:
    mutable std::mutex mutex;
    std::condition_variable waiter;
    std::deque<BlockPtr> queue;
    BlockPtr available;
    bool ended;

    BlockPtr dequeue(std::unique_lock<std::mutex>& lock, bool wait);

public:
    BlockQueue() : ended(false) { }
    ~BlockQueue();
    BlockQueue(const BlockQueue&) = delete;
    BlockQueue& operator=(const BlockQueue&) = delete;

    BlockPtr Add(BlockPtr& Filled); // Returns new/recycled block to fill.
    // These return nullptrs if nothing present.
    BlockPtr Remove(BlockPtr& Emptied, bool WaitForBlock = false);
    BlockPtr Remove(bool WaitForBlock = false);

    void End();
    bool Ended() const;

    bool Empty() const;
    size_t Size() const;
};


#endif /* BlockQueue_hpp */
