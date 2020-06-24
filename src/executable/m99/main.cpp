#include <library/m99.h>
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

//#define VERBOSE_MAIN


namespace
{

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
    void encode
    (
        std::uint8_t const * begin,
        std::uint8_t const * end,
        std::ofstream & outStream,
        std::uint32_t numThreads
    )
    {        
        std::uint32_t inputSize = std::distance(begin, end);

        auto sentinelIndex = maniscalco::forward_burrows_wheeler_transform(begin, end, numThreads);

        std::vector<std::thread> encodingThreads;
        std::vector<std::vector<std::uint8_t>> encodeBuffers(numThreads);
        std::uint32_t bytesPerBlock = ((inputSize + numThreads - 1) / numThreads);
        auto cur = begin;
        for (std::uint32_t i = 0; i < numThreads; ++i)
        {
            std::thread t([](std::uint8_t const * begin, std::uint8_t const * end, std::vector<std::uint8_t> & encodeBuffer)
                    {
                        auto e = maniscalco::m99_encode(begin, end);
                        encodeBuffer = std::move(e);
                    },
                    cur, (cur + bytesPerBlock) > end ? end : (cur + bytesPerBlock), std::ref(encodeBuffers[i]));
            encodingThreads.push_back(std::move(t));
            cur += bytesPerBlock;
        }
        for (auto & e : encodingThreads)
            e.join();


        std::size_t outputSize = 0;
        outputSize = (sizeof(inputSize) + sizeof(sentinelIndex) + sizeof(bytesPerBlock));
        for (auto const & e : encodeBuffers)
        {
            outputSize += e.size();
            outputSize += sizeof(std::uint32_t);  
        }

        std::uint32_t totalEncodedSize = sizeof(inputSize) + sizeof(sentinelIndex) + sizeof(bytesPerBlock);
        for (auto const & e : encodeBuffers)
        {
            totalEncodedSize += e.size();
            totalEncodedSize += sizeof(std::uint32_t);
        }
        outStream.write((char *)&totalEncodedSize, sizeof(totalEncodedSize));

        outStream.write((char *)&inputSize, sizeof(inputSize));
        outStream.write((char *)&sentinelIndex, sizeof(sentinelIndex));
        outStream.write((char *)&bytesPerBlock, sizeof(bytesPerBlock));
        for (auto const & e : encodeBuffers)
        {
            std::uint32_t encodedSize = e.size();
            outStream.write((char const *)&encodedSize, sizeof(encodedSize));
            outStream.write((char const *)&*e.begin(), encodedSize);
        }
    }


    //==================================================================================================================
    void decode
    (
        std::uint8_t const * begin,
        std::ofstream & outStream,
        std::uint32_t numThreads
    )
    {
        std::uint32_t originalSize = *(std::uint32_t const *)&*begin;
        begin += sizeof(originalSize);
        std::uint32_t sentinelIndex = *(std::uint32_t const *)&*begin;
        begin += sizeof(sentinelIndex);
        std::uint32_t blockSize = *(std::uint32_t const *)&*begin;
        begin += sizeof(blockSize);
        std::vector<std::uint8_t> decodedData;
        decodedData.resize(originalSize);

        auto inputCurrent = begin;
        auto outputCurrent = decodedData.data();
        auto bytesToDecode = originalSize;


        std::array<int32_t, 256> symbolCount;
        for (auto & e : symbolCount)
            e = 0;

        while (bytesToDecode > 0)
        {
            std::vector<std::thread> decodeThreads;
            for (uint32_t i = 0; i < numThreads; ++i)
            {
                if (blockSize > bytesToDecode)
                    blockSize = bytesToDecode;
                if (bytesToDecode > 0)
                {
                    std::uint32_t encodedSize = *(std::uint32_t const *)inputCurrent;
                    inputCurrent += sizeof(encodedSize);
                    std::thread t([](std::uint8_t const * inputBegin, std::uint8_t const * inputEnd, std::uint8_t * outputBegin, std::uint8_t * outputEnd)
                            {maniscalco::m99_decode(inputBegin, inputEnd, outputBegin, outputEnd);}, 
                            inputCurrent, inputCurrent + encodedSize, outputCurrent, outputCurrent + blockSize);
                    decodeThreads.push_back(std::move(t));
                    inputCurrent += encodedSize;
                    outputCurrent += blockSize;
                    bytesToDecode -= blockSize;
                }
            }
            for (auto & e : decodeThreads)
                e.join();
        }
        maniscalco::reverse_burrows_wheeler_transform(decodedData.begin(), decodedData.end(), sentinelIndex, numThreads);
        

        outStream.write((char const *)&*decodedData.begin(), decodedData.size());
     //   outStream.close();
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

        std::vector<std::uint8_t> input;
        input.reserve(1 << 30);

        auto startTime = std::chrono::system_clock::now();
        while (true)
        {
            std::uint32_t blockSize = 0;
            inputStream.read((char *)&blockSize, sizeof(blockSize));
            if (inputStream.gcount() == 0)
                break;
            input.resize(blockSize);
            inputStream.read((char *)input.data(), blockSize);
            decode(input.data(), outStream, numThreads);
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
            encode(input.data(), input.data() + size, outStream, numThreads);
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
//            auto input = load_file(argValue[2]);
  //          encode((std::uint8_t const *)input.data(), (std::uint8_t const *)input.data() + input.size(), argValue[3], numThreads);
            break;
        }

        case 'd':
        {
            decode(argValue[2], argValue[3], numThreads);
//            auto input = load_file(argValue[2]);
  //          decode((std::uint8_t const *)input.data(), argValue[3], numThreads);
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

