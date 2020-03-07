#include "FileDescriptorInput.hpp"
#include "BlockQueue.hpp"

#define ENQUOTE(x) ENQUOTE_1(x)
#define ENQUOTE_1(x) #x
#define INCLUDE_FILE(x) ENQUOTE(x)

#include INCLUDE_FILE(HEADER)
#include <iostream>

const size_t block_size = 32768;

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
        std::cout << count << '\n';
        buffer->resize(count + 1);
        buffer->back() = 0;
        buffer = Storage.Add(buffer);
    }
    Storage.End();
}

void parse_input(BlockQueue& Storage) {
    ParserPool pp;
    ReadFloatArrayIO parser;
    do {
        BlockQueue::BlockPtr block(Storage.Remove());
        if (!block)
            break;
        parser.Scan(&block->front(), &block->back(), pp);
    } while (!parser.Finished());
    ReadFloatArrayIOValues out;
    parser.Swap(out.values);
}

int main(int argc, char** argv) {
    BlockQueue read;
    FileDescriptorInput input(0);
    read_input(input, read);
    parse_input(read);
    return 0;
}
