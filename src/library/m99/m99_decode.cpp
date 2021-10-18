#include "./m99_decode.h"

#include <fstream>


namespace maniscalco
{
    template <typename T>
    class m99_decoder
    {
    public:

        using symbol_type = T;
        static auto constexpr max_symbol_count = (1ull << (sizeof(T) * 8));


        m99_decoder()
        {
            leftStack_ = &left_[0];
            rightStack_ = &right_[0];
        }

        void decode
        (
            m99_decode_stream &,
            symbol_type *,
            symbol_type *
        );

    private:

        struct symbol_info
        {
            symbol_info(){}
            symbol_info(T symbol, std::uint32_t count):symbol_(symbol), count_(count){}
            T               symbol_;
            std::uint32_t   count_;
        };

        void split
        (
            m99_decode_stream &,
            symbol_type *,
            std::uint32_t,
            std::uint32_t,
            symbol_info const *
        );

        symbol_info left_[22 * max_symbol_count];
        symbol_info right_[22 * max_symbol_count];

        symbol_info * leftStack_;
        symbol_info * rightStack_;
    };

}


namespace
{
    using namespace maniscalco;


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

}


//======================================================================================================================
template <typename T>
void maniscalco::m99_decoder<T>::split
(
    m99_decode_stream & decodeStream,
    T * decodedData,
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

    auto leftSymbolInfo = leftStack_;
    auto rightSymbolInfo = rightStack_;

    leftStack_ += max_symbol_count;
    rightStack_ += max_symbol_count;

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

    leftStack_ -= max_symbol_count;
    rightStack_ -= max_symbol_count;
}


//=============================================================================
template <typename T>
void maniscalco::m99_decoder<T>::decode
(
    m99_decode_stream & decodeStream,
    T * outputBegin,
    T * outputEnd
)
{
    while (!decodeStream.pop(1))
        ; // pop until a 1 bit is decoded. this is start of stream marker.

    // decode the header stream
    symbol_info symbolInfo[max_symbol_count];
    auto bytesToDecode = std::distance(outputBegin, outputEnd);
    auto n = bytesToDecode;
    for (auto i = 0; i < max_symbol_count; ++i)
    {
        if (n == 0)
            break;
        symbolInfo[i].count_ = unpack_value(decodeStream, n, n, n);
        symbolInfo[i].symbol_ = decodeStream.pop(sizeof(T) * 8);
        n -= symbolInfo[i].count_;
    }
    
    std::uint32_t leftSize = 1;
    while (leftSize < bytesToDecode)
        leftSize <<= 1;
    split(decodeStream, outputBegin, bytesToDecode, leftSize >> 1, symbolInfo);
}


//=============================================================================
template <typename T>
void maniscalco::m99_decode
(
    m99_decode_stream & decodeStream,
    T * outputBegin,
    T * outputEnd
)
{
    using decoder_type = m99_decoder<T>;

    std::unique_ptr<decoder_type> m99Decoder(new decoder_type);
    m99Decoder->decode(decodeStream, outputBegin, outputEnd);
}


//=============================================================================
namespace maniscalco
{

    template void m99_decode(m99_decode_stream &, std::uint16_t *, std::uint16_t *);
    template void m99_decode(m99_decode_stream &, std::uint8_t *, std::uint8_t *);

}