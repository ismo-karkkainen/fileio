//
//  ThreadedReadParse_t.hpp
//  imageio
//
//  Created by Ismo Kärkkäinen on 16.3.2020.
//  Copyright (c) 2020 Ismo Kärkkäinen. All rights reserved.
//
// Licensed under Universal Permissive License. See License.txt.

#if !defined(THREADEDREADPARSE_T_HPP)
#define THREADEDREADPARSE_T_HPP

#include "BlockQueue.hpp"
#include "InputChannel.hpp"
#include <thread>
#include <ctime>


// Reads exactly one input and parses it. Has copied output if Finished().
template<typename Parser, typename Values>
class ThreadedReadParse {
private:
    BlockQueue read;
    InputChannel& input;
    std::thread* worker;
    bool finished;

    void nap() {
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 20000000;
        nanosleep(&ts, nullptr);
    }

    size_t block_size() const {
        return 1048576;
    }

    void reader() {
        BlockQueue::BlockPtr buffer;
        while (!input.Ended() && !finished) {
            if (!buffer)
                buffer.reset(new BlockQueue::Block());
            if (buffer->size() != block_size() + 1)
                buffer->resize(block_size() + 1);
            int count = input.Read(&buffer->front(), block_size());
            if (count == 0) {
                nap();
                continue;
            }
            buffer->resize(count + 1);
            buffer->back() = 0;
            buffer = read.Add(buffer);
        }
        read.End();
    }

public:
    ThreadedReadParse(InputChannel& In, Values& V)
        : input(In), worker(nullptr), finished(false)
    {
        worker = new std::thread(
            &ThreadedReadParse<Parser,Values>::reader, this);
        std::this_thread::yield();
        ParserPool pp;
        Parser p;
        BlockQueue::BlockPtr block(read.Remove());
        while (true) {
            if (!block) {
                nap();
                block = read.Remove();
                if (!block && read.Ended())
                    break;
                continue;
            }
            p.Scan(&block->front(), &block->back(), pp);
            block = read.Remove(block);
            if (!block && read.Ended())
                break; // There will be no more data.
            if (p.Finished())
                break;
        }
        finished = p.Finished();
        if (finished)
            p.Swap(V.values);
        while (read.Remove()); // Clear unused read blocks, if any.
        worker->join();
        delete worker;
        worker = nullptr;
    }

    bool Finished() const { return finished; }
};


#endif
