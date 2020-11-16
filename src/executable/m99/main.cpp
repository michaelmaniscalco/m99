#include <library/m99/m99_encode.h>
#include <library/m99/m99_decode.h>
#include <library/msufsort.h>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
#include <fstream>
#include <chrono>
#include <thread>
#include <cstring>
#include <string>
#include <atomic>
#include <mutex>


namespace
{

    static auto constexpr max_encode_block_size = (1ull << 20);

    struct block_header
    {
        std::uint32_t blockSize_;
        std::uint32_t sentinelIndex_;
    };


    //==================================================================================================================
    std::vector<char> load_file
    (
        char const * path
    )
    {
        // read data from file
        std::vector<char> input;
        std::ifstream inputStream(path, std::ios_base::in | std::ios_base::binary);
        if (!inputStream.is_open())
        {
            std::cout << "failed to open file \"" << path << "\"" << std::endl;
            return std::vector<char>();
        }

        inputStream.seekg(0, std::ios_base::end);
        std::size_t size = inputStream.tellg();
        input.resize(size);
        inputStream.seekg(0, std::ios_base::beg);
        inputStream.read(input.data(), input.size());
        inputStream.close();
        return input;
    }


    //==================================================================================================================
    void encode_block
    (
        // single threaded
        std::uint8_t const * inputBegin,
        std::uint8_t const * inputEnd,
        std::ofstream & outStream
    )
    {
        // transform input (BWT)
        auto sentinelIndex = maniscalco::forward_burrows_wheeler_transform(inputBegin, inputEnd, 1);

        // write header for input
        block_header blockHeader
        {
            .blockSize_ = std::distance(inputBegin, inputEnd),
            .sentinelIndex_ = sentinelIndex
        };
        outStream.write((char const *)&blockHeader, sizeof(blockHeader));

        std::uint32_t subBlockId{0};
        // encode next available sub block until there are none remaining
        std::uint32_t currentSubBlockId = subBlockId++;
        auto blockBegin = inputBegin + (currentSubBlockId * max_encode_block_size);
        while (blockBegin < inputEnd)
        {
            auto blockEnd = (blockBegin + max_encode_block_size);
            if (blockEnd > inputEnd)
                blockEnd = inputEnd;
            // create encode stream and encode this subblock
            maniscalco::m99_encode_stream encodeStream;
            maniscalco::m99_encode(blockBegin, blockEnd, encodeStream);
            encodeStream.flush();
            // write this encoded sub block to the destination
            auto encodedSize = ((encodeStream.size() + 7) / 8);
            outStream.write((char const *)&encodedSize, 4);
            outStream.write((char const *)&currentSubBlockId, 4);
            // write the encoded data for the stream
            for (auto const & packet : encodeStream)
            {
                auto bytesToWrite = ((packet.size() + 7) / 8);
                auto address = (packet.data() + packet.capacity() - bytesToWrite);
                outStream.write((char const *)address, bytesToWrite);
            }
            currentSubBlockId = subBlockId++;
            blockBegin = inputBegin + (currentSubBlockId * max_encode_block_size);
        }
    }



    //==================================================================================================================
    void encode_block
    (
        std::uint8_t const * inputBegin,
        std::uint8_t const * inputEnd,
        std::ofstream & outStream,
        std::size_t numThreads
    )
    {
        // transform input (BWT)
        auto sentinelIndex = maniscalco::forward_burrows_wheeler_transform(inputBegin, inputEnd, numThreads);

        // write header for input
        block_header blockHeader
        {
            .blockSize_ = std::distance(inputBegin, inputEnd),
            .sentinelIndex_ = sentinelIndex
        };
        outStream.write((char const *)&blockHeader, sizeof(blockHeader));

        // create worker threads for encoding
        std::vector<std::thread> threads;
        threads.resize(numThreads);
        
        // set threads to process sub blocks of the input
        std::atomic<std::uint32_t> subBlockId{0};
        std::mutex mutex;
        for (auto & thread : threads)
        {
            thread = std::thread([&]()
            {
                // encode next available sub block until there are none remaining
                std::uint32_t currentSubBlockId = subBlockId++;
                auto blockBegin = inputBegin + (currentSubBlockId * max_encode_block_size);
                while (blockBegin < inputEnd)
                {
                    auto blockEnd = (blockBegin + max_encode_block_size);
                    if (blockEnd > inputEnd)
                        blockEnd = inputEnd;
                    // create encode stream and encode this subblock
                    maniscalco::m99_encode_stream encodeStream;
                    maniscalco::m99_encode(blockBegin, blockEnd, encodeStream);
                    encodeStream.flush();
                    // write this encoded sub block to the destination
                    std::lock_guard lockGuard(mutex);
                    auto encodedSize = ((encodeStream.size() + 7) / 8);
                    outStream.write((char const *)&encodedSize, 4);
                    outStream.write((char const *)&currentSubBlockId, 4);
                    // write the encoded data for the stream
                    for (auto const & packet : encodeStream)
                    {
                        auto bytesToWrite = ((packet.size() + 7) / 8);
                        auto address = (packet.data() + packet.capacity() - bytesToWrite);
                        outStream.write((char const *)address, bytesToWrite);
                    }
                    currentSubBlockId = subBlockId++;
                    blockBegin = inputBegin + (currentSubBlockId * max_encode_block_size);
                }
            });
        }
        // wait for threads to complete encoding
        for (auto & thread : threads)
            thread.join();
    }


    //==================================================================================================================
    void decode_block
    (
        std::ifstream & inStream,
        std::ofstream & outStream,
        std::uint32_t numThreads
    )
    {
        // read header for block
        block_header blockHeader;
        inStream.read((char *)&blockHeader, sizeof(blockHeader));
        std::uint32_t bytesPerSubBlock = ((blockHeader.blockSize_ + numThreads - 1) / numThreads);

        // allocate space for decoded block data
        std::vector<std::uint8_t> output;
        output.resize(blockHeader.blockSize_);
        auto outputBegin = output.data();
        auto outputEnd = (outputBegin + output.size());

        // create decode threads
        std::vector<std::thread> threads;
        threads.resize(numThreads - 1);

        std::atomic<std::uint32_t> numSubBlocksToDecode((blockHeader.blockSize_ + max_encode_block_size - 1) / max_encode_block_size);
        auto n = numSubBlocksToDecode.load();

        std::mutex mutex;
        for (auto & thread : threads)
        {
            thread = std::thread([&]()
                {
                    while (true)
                    {
                        maniscalco::buffer encodedData;
                        std::uint32_t encodedSize = 0;
                        std::uint32_t subBlockId = 0;
                        {
                            std::lock_guard lockGuard(mutex);
                            if (numSubBlocksToDecode < 1)
                                return; // no more work to do

                            --numSubBlocksToDecode;
                            // read next compress subblock from source
                            inStream.read((char *)&encodedSize, 4);
                            inStream.read((char *)&subBlockId, 4);
                            // read encoded sub block data
                            encodedData = std::move(maniscalco::buffer(encodedSize));
                            inStream.read((char *)encodedData.data(), encodedSize);
                        }
                        auto destinationBegin = (outputBegin + (subBlockId * max_encode_block_size));
                        auto destinationEnd = (destinationBegin + max_encode_block_size);
                        if (destinationEnd > outputEnd)
                            destinationEnd = outputEnd;
                        maniscalco::m99_decode_stream decodeStream(std::move(encodedData), encodedSize);
                        maniscalco::m99_decode(decodeStream, destinationBegin, destinationEnd);
                    }
                });
        }

        // wait for all subblocks to be decoded
        for (auto & thread : threads)
            thread.join();

        // reverse the BWT
        maniscalco::reverse_burrows_wheeler_transform(output.begin(), output.end(), blockHeader.sentinelIndex_, numThreads);
        outStream.write((char const *)&*outputBegin, output.size());
    }


    //==================================================================================================================
    void decode_block
    (
        // single threaded
        std::ifstream & inStream,
        std::ofstream & outStream
    )
    {
        // read header for block
        block_header blockHeader;
        inStream.read((char *)&blockHeader, sizeof(blockHeader));
        std::uint32_t bytesPerSubBlock = blockHeader.blockSize_;

        // allocate space for decoded block data
        std::vector<std::uint8_t> output;
        output.resize(blockHeader.blockSize_);
        auto outputBegin = output.data();
        auto outputEnd = (outputBegin + output.size());

        std::uint32_t numSubBlocksToDecode((blockHeader.blockSize_ + max_encode_block_size - 1) / max_encode_block_size);
        while (numSubBlocksToDecode-- > 0)
        {
            maniscalco::buffer encodedData;
            std::uint32_t encodedSize = 0;
            std::uint32_t subBlockId = 0;
            // read next compress subblock from source
            inStream.read((char *)&encodedSize, 4);
            inStream.read((char *)&subBlockId, 4);
            // read encoded sub block data
            encodedData = std::move(maniscalco::buffer(encodedSize));
            inStream.read((char *)encodedData.data(), encodedSize);
            auto destinationBegin = (outputBegin + (subBlockId * max_encode_block_size));
            auto destinationEnd = (destinationBegin + max_encode_block_size);
            if (destinationEnd > outputEnd)
                destinationEnd = outputEnd;
            maniscalco::m99_decode_stream decodeStream(std::move(encodedData), encodedSize);
            maniscalco::m99_decode(decodeStream, destinationBegin, destinationEnd);
        }
        // reverse the BWT
        maniscalco::reverse_burrows_wheeler_transform(output.begin(), output.end(), blockHeader.sentinelIndex_, 1);
        outStream.write((char const *)&*outputBegin, output.size());
    }


    //==================================================================================================================
    void print_about
    (
    )
    {
        std::cout << "m99 - high performance BWT compressor.  Author: M.A. Maniscalco (1999 - 2017)" << std::endl;
    }


    //==================================================================================================================
    std::int32_t print_usage
    (
    )
    {
        std::cout << "Usage: m99 [e|d] inputFile outputFile [switches]" << std::endl;
        std::cout << "\t -t = threadCount" << std::endl;
        std::cout << "\t -b = blockSize (max = 1GB)" << std::endl; 

        std::cout << "example: m99 e inputFile outputFile -t8 -b100000" << std::endl;
        std::cout << "example: m99 d inputFile outputFile -t8" << std::endl; 
        return 0;
    }


    //==========================================================================
    void decode
    (
        char const * inputPath,
        char const * outputPath,
        int numThreads
    )
    {
        // read data from file
        std::ifstream inputStream(inputPath, std::ios_base::in | std::ios_base::binary);
        if (!inputStream.is_open())
        {
            std::cout << "failed to open file \"" << inputPath << "\"" << std::endl;
            return;
        }

        // create the output stream
        std::ofstream outStream(outputPath, std::ios_base::out | std::ios_base::binary);
        if (!outStream.is_open())
        {
            std::cout << "failed to create output file \"" << outputPath << "\"" << std::endl;
            return;
        }

        auto startTime = std::chrono::system_clock::now();
        inputStream.seekg(0, std::ios_base::end);
        auto end = inputStream.tellg();
        inputStream.seekg(0, std::ios_base::beg);

        if (numThreads == 1)
        {
            while (inputStream.tellg() != end)
                decode_block(inputStream, outStream);
        }
        else
        {
            while (inputStream.tellg() != end)
                decode_block(inputStream, outStream, numThreads);
        }

        auto finishTime = std::chrono::system_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(finishTime - startTime).count();
        std::cout << "Elapsed time: " << ((long double)elapsedTime / 1000) << " seconds" << std::endl;

        inputStream.close();
        outStream.close();
    }


    //=================================================================================
    void encode
    (
        char const * inputPath,
        char const * outputPath,
        int numThreads,
        int blockSize
    )
    {
        // create the output stream
        std::ofstream outStream(outputPath, std::ios_base::out | std::ios_base::binary);
        if (!outStream.is_open())
        {
            std::cout << "failed to create output file \"" << outputPath << "\"" << std::endl;
            return;
        }

        auto startTime = std::chrono::system_clock::now();

        // read data from file
        std::vector<std::uint8_t> input;
        input.reserve(blockSize);
        std::ifstream inputStream(inputPath, std::ios_base::in | std::ios_base::binary);
        if (!inputStream.is_open())
        {
            std::cout << "failed to open file \"" << inputPath << "\"" << std::endl;
            return;
        }

        std::size_t bytesEncoded = 0;
        inputStream.seekg(0, std::ios_base::beg);
        while (true)
        {
            inputStream.read((char *)input.data(), input.capacity());
            auto size = inputStream.gcount();
            if (size == 0)
                break;
            bytesEncoded += size;
            if (numThreads == 1)
                encode_block(input.data(), input.data() + size, outStream);
            else
                encode_block(input.data(), input.data() + size, outStream, numThreads);
        }
        auto finishTime = std::chrono::system_clock::now();
        auto elapsedOverallEncode = std::chrono::duration_cast<std::chrono::milliseconds>(finishTime - startTime).count();

        std::size_t inputSize = bytesEncoded;
        std::size_t outputSize = outStream.tellp();

        std::cout << "compressed: " << inputSize << " -> " << outputSize << " bytes.  ratio = " << (((long double)outputSize / inputSize) * 100) << "%" << std::endl;
        std::cout << "Elapsed time: " << ((long double)elapsedOverallEncode / 1000) << " seconds : " <<  (((long double)inputSize / (1 << 20)) / ((double)elapsedOverallEncode / 1000)) << " MB/sec" << std::endl;

        outStream.close();
        inputStream.close();
    }

}


//======================================================================================================================
std::int32_t main
(
    std::int32_t argCount,
    char const * argValue[]
)
{
    print_about();

    if ((argCount < 4) || (strlen(argValue[1]) != 1))
        return print_usage();

    std::size_t numThreads = 0;
    std::size_t maxBlockSize = (1 << 30);
    for (auto argIndex = 4; argIndex < argCount; ++argIndex)
    {
        if (argValue[argIndex][0] != '-')
            return print_usage();

        switch (argValue[argIndex][1])
        {
            case 'b':
            {
                // block size
                maxBlockSize = 0;
                auto cur = argValue[argIndex] + 2;
                while (*cur != 0)
                {
                    if ((*cur < '0') || (*cur > '9'))
                    {
                        std::cout << "invalid block size" << std::endl;
                        print_usage();
                        return -1;
                    }
                    maxBlockSize *= 10;
                    maxBlockSize += (*cur - '0');
                    ++cur;
                }
                if (maxBlockSize > (1 << 30))
                    maxBlockSize = (1 << 30);
                break;
            }
            case 't':
            {
                // thread count
                numThreads = 0;
                auto cur = argValue[argIndex] + 2;
                while (*cur != 0)
                {
                    if ((*cur < '0') || (*cur > '9'))
                    {
                        std::cout << "invalid thread count" << std::endl;
                        print_usage();
                        return -1;
                    }
                    numThreads *= 10;
                    numThreads += (*cur - '0');
                    ++cur;
                }
                break;
            }
            default:
            {
                std::cout << "unknown switch: " << argValue[argIndex] << std::endl;
                return print_usage();
            }
        }
    }
    if ((numThreads == 0) || (numThreads > std::thread::hardware_concurrency()))
        numThreads = std::thread::hardware_concurrency();

    switch (argValue[1][0])
    {
        case 'e':
        {
            encode(argValue[2], argValue[3], numThreads, maxBlockSize);
            break;
        }

        case 'd':
        {
            decode(argValue[2], argValue[3], numThreads);
            break;
        }

        default:
        {
            print_usage();
            break;
        }
    }

    return 0;
}

