#include "./m99_decode.h"

#include <fstream>


namespace
{
    using namespace maniscalco;

    struct symbol_info
    {
        symbol_info(){}
        symbol_info(std::uint8_t symbol, std::uint32_t count):symbol_(symbol), count_(count){}
        std::uint8_t    symbol_;
        std::uint32_t   count_;
    };


    //======================================================================================================================
    std::uint32_t unpack_value
    (
        m99_decode_stream & decodeStream,
        std::uint32_t total,
        std::uint32_t maxLeft,
        std::uint32_t maxRight
    )
    {
        if (total > maxLeft)
        {
            auto inferredRight = (total - maxLeft);
            maxRight -= inferredRight;
            total -= inferredRight;
        }
        auto left = 0;
        if (total > maxRight)
        {
            left = (total - maxRight);
            total -= left;
        }
        if (total)
        {
            std::uint32_t codeLength = 1;
            while (total >> ++codeLength)
                ;
            auto code = decodeStream.pop(--codeLength);
            if (((code | (1ull << codeLength)) <= total))
                code |= (decodeStream.pop_bit() << codeLength);
            left += code;
        }    
        return left;
    }


    //======================================================================================================================
    void split
    (
        m99_decode_stream & decodeStream,
        std::uint8_t * decodedData,
        std::uint32_t totalSize,
        std::uint32_t leftSize,
        symbol_info const * parentSymbolInfo
    )
    {
        if (parentSymbolInfo[0].count_ >= totalSize)
        {
            while (totalSize--)
                *decodedData++ = parentSymbolInfo[0].symbol_;
            return;
        }

        if (totalSize <= 2)
        {
            if (totalSize == 2)
            {
                auto c = decodeStream.pop_bit();
                decodedData[c == 1] = parentSymbolInfo[1].symbol_;
                decodedData[c == 0] = parentSymbolInfo[0].symbol_; 
            }
            else
            {
                decodedData[0] = parentSymbolInfo[0].symbol_;
            }
            return;
        }

        std::uint32_t rightSize = (totalSize - leftSize);
        symbol_info leftSymbolInfo[256];
        symbol_info rightSymbolInfo[256];
        symbol_info * result[2] = {leftSymbolInfo, rightSymbolInfo};
        symbol_info const * currentSymbolInfo = parentSymbolInfo;
        static auto constexpr leftSide = 0;
        static auto constexpr rightSide = 1;

        auto leftSizeRemaining = leftSize;
        auto rightSizeRemaining = rightSize;
        while (leftSizeRemaining && rightSizeRemaining)
        {
            symbol_info symbolInfo = *currentSymbolInfo++;
            auto totalCount = symbolInfo.count_;
            auto leftCount = unpack_value(decodeStream, totalCount, leftSizeRemaining, rightSizeRemaining);
            auto rightCount = (totalCount - leftCount);
            leftSizeRemaining -= leftCount;
            rightSizeRemaining -= rightCount;
            *result[leftSide] = {symbolInfo.symbol_, leftCount};
            *result[rightSide] = {symbolInfo.symbol_, rightCount};
            result[leftSide] += (leftCount != 0);
            result[rightSide] += (rightCount != 0);
        }
        auto n = leftSizeRemaining + rightSizeRemaining;
        symbol_info * c = result[(leftSizeRemaining == 0)];
        while (n > 0)
        {
            n -= currentSymbolInfo->count_;
            *c++ = *currentSymbolInfo++;
        }
        split(decodeStream, decodedData, leftSize, leftSize >> 1, leftSymbolInfo);
        split(decodeStream, decodedData + leftSize, rightSize, rightSize >> 1, rightSymbolInfo);
    }


} // namespace


//======================================================================================================================
void maniscalco::m99_decode
(
    m99_decode_stream & decodeStream,
    std::uint8_t * outputBegin,
    std::uint8_t * outputEnd
)
{
    while (!decodeStream.pop(1))
        ; // pop until a 1 bit is decoded. this is start of stream marker.

    // decode the header stream
    symbol_info symbolInfo[256];
    auto bytesToDecode = std::distance(outputBegin, outputEnd);
    auto n = bytesToDecode;
    for (auto i = 0; i < 256; ++i)
    {
        if (n == 0)
            break;
        symbolInfo[i].count_ = unpack_value(decodeStream, n, n, n);
        symbolInfo[i].symbol_ = decodeStream.pop(8);
        n -= symbolInfo[i].count_;
    }
    
    std::uint32_t leftSize = 1;
    while (leftSize < bytesToDecode)
        leftSize <<= 1;
    split(decodeStream, outputBegin, bytesToDecode, leftSize >> 1, symbolInfo);
}

