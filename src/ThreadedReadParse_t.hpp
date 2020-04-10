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
#include <deque>
#include <mutex>
#include <memory>
#include <iostream>


template<typename Parser, typename Values>
class ThreadedReadParse {
private:
    BlockQueue read;
    InputChannel& input;
    std::thread* worker;
    std::thread* parseworker;
    std::deque<std::shared_ptr<Values>>& queue;
    std::mutex& mutex;
    std::condition_variable& output_waiter;
    volatile bool finished;

    void nap() const {
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
            if (16 * 1024 * 1024 < read.Size() * block_size()) {
                nap(); // Parsing can not keep up so pause reading.
                continue;
            }
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

    void parser() {
        ParserPool pp;
        Parser p;
        BlockQueue::BlockPtr block(read.Remove());
        const char* end = nullptr;
        while (!finished) {
            if (end == nullptr) {
                block = read.Remove(block);
                if (!block) {
                    if (read.Ended())
                        break;
                    nap();
                    continue;
                }
                end = &block->front();
            }
            if (p.Finished()) {
                end = pp.skipWhitespace(end, &block->back());
                if (end == nullptr)
                    continue;
            }
            try {
                end = p.Scan(end, &block->back(), pp);
            }
            catch (const ParserException& e) {
                std::cerr << e.what() << std::endl;
                finished = true;
                continue;
            }
            if (!p.Finished()) {
                end = nullptr;
                continue;
            }
            std::shared_ptr<Values> v(new Values());
            p.Swap(v->values);
            std::unique_lock<std::mutex> lock(mutex);
            queue.push_back(v);
            lock.unlock();
            output_waiter.notify_one();
        }
        while (read.Remove()); // Clear unused read blocks, if any.
        finished = true;
        output_waiter.notify_one();
    }

    void finish() {
        finished = true;
        worker->join();
        delete worker;
        worker = nullptr;
        parseworker->join();
        delete parseworker;
        parseworker = nullptr;
    }

public:
    ThreadedReadParse(InputChannel& In, std::deque<std::shared_ptr<Values>>& Q,
        std::mutex& M, std::condition_variable& OutputWaiter)
        : input(In), worker(nullptr), queue(Q), mutex(M),
        output_waiter(OutputWaiter), finished(false)
    {
        worker = new std::thread(
            &ThreadedReadParse<Parser,Values>::reader, this);
        parseworker = new std::thread(
            &ThreadedReadParse<Parser,Values>::parser, this);
    }

    ~ThreadedReadParse() { finish(); }

    bool Finished() const { return finished; }

    void Nap() const { nap(); }
};


#endif
