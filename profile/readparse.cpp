#define STR(x) #x
#define ENQUOTE(x) STR(x)
#define INCLUDE_FILE(x) ENQUOTE(x)
#include INCLUDE_FILE(HEADER)
#include "FileDescriptorInput.hpp"
#include "BlockQueue.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>


const size_t block_size = 1048576;

void read_input(InputChannel& Input, BlockQueue& Storage) {
    BlockQueue::BlockPtr buffer;
    while (!Input.Ended()) {
        if (!buffer)
            buffer.reset(new BlockQueue::Block());
        if (buffer->size() != block_size + 1)
            buffer->resize(block_size + 1);
        int count = Input.Read(&buffer->front(), block_size);
        if (count == 0)
            continue;
        //std::cout << count << '\n';
        buffer->resize(count + 1);
        buffer->back() = 0;
        buffer = Storage.Add(buffer);
    }
    Storage.End();
}

void parse_input(BlockQueue& Storage) {
    ParserPool pp;
    ReadSomething parser;
    do {
        BlockQueue::BlockPtr block(Storage.Remove());
        if (!block)
            break;
        parser.Scan(&block->front(), &block->back(), pp);
    } while (!parser.Finished());
    ReadSomethingValues out;
    parser.Swap(out.values);
}

int main(int argc, char** argv) {
    int f = 0;
    if (argc > 1)
        f = open(argv[1], O_RDONLY);
    BlockQueue read;
    FileDescriptorInput input(f);
    read_input(input, read);
    if (f)
        close(f);
    parse_input(read);
    return 0;
}
