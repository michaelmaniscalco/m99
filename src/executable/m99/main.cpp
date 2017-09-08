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
            std::cout << "failed to open configuration file \"" << path << "\"" << std::endl;
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
    template <typename input_iter>
    void encode
    (
        input_iter begin,
        input_iter end,
        char const * outputPath,
        std::size_t numThreads
    )
    {        
        auto inputSize = std::distance(begin, end);

        // create the output stream
        std::ofstream outStream(outputPath, std::ios_base::out | std::ios_base::binary);
        if (!outStream.is_open())
        {
            std::cout << "failed to create output file \"" << outputPath << "\"" << std::endl;
            return;
        }

        // do the forward BWT
        std::cout << "Performing forward BWT ..." << std::flush << (char)13;
        auto startBWT = std::chrono::system_clock::now();
        auto sentinelIndex = maniscalco::forward_burrows_wheeler_transform(begin, end, numThreads);
        auto finishBWT = std::chrono::system_clock::now();
        auto elapsedBWT = std::chrono::duration_cast<std::chrono::milliseconds>(finishBWT - startBWT).count();
        std::cout << "BWT completed - " << (((long double)inputSize / (1 << 20)) / ((double)elapsedBWT / 1000)) << 
                " MB/sec            " << std::endl;

        // encode the transformed input
        std::cout << "Encoding " << inputSize << " bytes ..." << std::flush << (char)13;
        auto startEncode = std::chrono::system_clock::now();
        auto encodedData = maniscalco::m99_encode((std::uint8_t const *)&*begin, (std::uint8_t const *)&*end);
        auto finishEncode = std::chrono::system_clock::now();
        auto elapsedEncode = std::chrono::duration_cast<std::chrono::milliseconds>(finishEncode - startEncode).count();
        std::cout << "Encode completed - " << (((long double)inputSize / (1 << 20)) / ((double)elapsedEncode / 1000)) << 
                " MB/sec          " << std::endl;

        // write the data to the output stream
        std::cout << "Creating compressed file ... " << outputPath << std::flush << (char)13;
        outStream.write((char *)&sentinelIndex, sizeof(sentinelIndex));
        outStream.write((char const *)&*encodedData.begin(), encodedData.size());
        std::size_t outputSize = outStream.tellp();
        outStream.close();

        // report encode speed (excluding bwt time)
        std::cout << "Encoded size = " << outputSize << " bytes.  Compression = " <<
                (((long double)outputSize / inputSize) * 100) << "%" << std::endl << std::endl;
    }


    //==================================================================================================================
    template <typename input_iter>
    void decode
    (
        input_iter begin,
        input_iter end,
        char const * outputPath
    )
    {
        std::uint32_t sentinelIndex = *(std::uint32_t const *)&*begin;
        begin += 4;

        auto startDecode = std::chrono::system_clock::now();
        auto decodedData = maniscalco::m99_decode((std::uint8_t const *)&*begin, (std::uint8_t const *)&*end);
        auto finishDecode = std::chrono::system_clock::now();
        auto elapsedDecode = std::chrono::duration_cast<std::chrono::milliseconds>(finishDecode - startDecode).count();
        std::cout << "Decode speed: " << (((long double)decodedData.size() / (1 << 20)) / ((double)elapsedDecode / 1000)) << " MB/sec" << std::endl;

        auto startBWT = std::chrono::system_clock::now();
        maniscalco::reverse_burrows_wheeler_transform(decodedData.begin(), decodedData.end(), sentinelIndex);
        auto finishBWT = std::chrono::system_clock::now();
        auto elapsedBWT = std::chrono::duration_cast<std::chrono::milliseconds>(finishBWT - startBWT).count();
        std::cout << "UnBWT completed - " << (((long double)decodedData.size() / (1 << 20)) / ((double)elapsedBWT / 1000)) << " MB/sec" << std::endl;
        
        // create the output stream
        std::ofstream outStream(outputPath, std::ios_base::out | std::ios_base::binary);
        if (!outStream.is_open())
        {
            std::cout << "failed to create output file \"" << outputPath << "\"" << std::endl;
            return;
        }
        outStream.write((char const *)&*decodedData.begin(), decodedData.size());
        std::size_t outputSize = outStream.tellp();
        outStream.close();

        std::cout << "Decoded size = " << outputSize << " bytes" << std::endl << std::endl;
    }


    //==================================================================================================================
    void print_about
    (
    )
    {
        std::cout << "M99 compressor demo" << std::endl;
        std::cout << "Author: M.A. Maniscalco" << std::endl << std::endl;
    }


    //==================================================================================================================
    std::int32_t print_usage
    (
    )
    {
        std::cout << "Usage: m99 [e|d] inputFile outputFile" << std::endl;
        return 0;
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

    if ((argCount != 4) || (strlen(argValue[1]) != 1))
        return print_usage();

    switch (argValue[1][0])
    {
        case 'e':
        {
            auto input = load_file(argValue[2]);
            encode(input.begin(), input.end(), argValue[3], std::thread::hardware_concurrency());
            break;
        }

        case 'd':
        {
            auto input = load_file(argValue[2]);
            decode(input.begin(), input.end(), argValue[3]);
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

