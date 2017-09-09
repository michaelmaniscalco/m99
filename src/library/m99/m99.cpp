#include "./m99.h"
#include "./input_stream.h"
#include "./output_stream.h"




namespace
{
    using namespace maniscalco;

    struct symbol_info
    {
        std::uint8_t    symbol_;
        std::size_t     count_;
    };

    struct tiny_encode_table_entry_type
    {
        std::size_t value_;
        std::size_t length_;
    };

    using tiny_encode_table_type = tiny_encode_table_entry_type[8][8][8][8];


    tiny_encode_table_type tinyEncodeTable;
    auto const initialize = []
    (
        tiny_encode_table_type & result
    ) -> bool
    {
        for (std::size_t maxLeft = 0; maxLeft < 8; ++maxLeft)
        {
            for (std::size_t maxRight = 0; maxRight < 8; ++maxRight)
            {
                for (std::size_t left = 0; left <= maxLeft; ++left)
                {
                    for (std::size_t right = 0; right <= maxRight; ++right)
                    {
                        std::size_t total = left + right;
                        if (total < 8)
                        {
                            std::size_t l = left;
                            std::size_t r = right;
                            std::size_t ml = maxLeft;
                            std::size_t mr = maxRight;
                            std::size_t t = (l + r);
                            if (t > ml)
                            {
                                std::size_t inferredRight = (t - ml);
                                mr -= inferredRight;
                                t -= inferredRight;
                            }
                            if (t > mr)
                            {
                                std::size_t inferredLeft = (t - mr);
                                l -= inferredLeft;
                                t -= inferredLeft;
                            }
                            std::size_t codeLength = 0;
                            while ((1ull << ++codeLength) <= t)
                                ;
                            --codeLength;
                            auto needMsb = ((l | (1ull << codeLength)) <= t);
                            auto code = ((l << needMsb) | (l >> codeLength));
                            codeLength += needMsb;
                            result[maxLeft][maxRight][left][total] = {code, codeLength};
                        }
                    }
                }
            }
        }
        return true;
    }(tinyEncodeTable);


    //======================================================================================================================
    inline void pack_value
    (
        output_stream * dataStream,
        std::size_t left,
        std::size_t total,
        std::size_t maxLeft,
        std::size_t maxRight
    )
    {
        if (total < 8)
        {
            auto const & encTableEntry = tinyEncodeTable[(maxLeft >= 8) ? 7 : maxLeft][(maxRight >= 8) ? 7 : maxRight][left][total];
            dataStream->push(encTableEntry.value_, encTableEntry.length_);
            return;
        }
        if (total > maxLeft)
        {
            auto inferredRight = (total - maxLeft);
            maxRight -= inferredRight;
            total -= inferredRight;
        }
        if (total > maxRight)
        {
            auto inferredLeft = (total - maxRight);
            left -= inferredLeft;
            total -= inferredLeft;
        }
        if (total)
        {
            std::size_t codeLength = 1;
            while (total >> ++codeLength)
                ;
            --codeLength;
            auto needMsb = ((left | (1ull << codeLength)) <= total);
            auto code = ((left << needMsb) | (left >> codeLength));
            codeLength += needMsb;
            dataStream->push(code, codeLength);
        }
    }


    //======================================================================================================================
    inline std::size_t unpack_value
    (
        input_stream * inputStream,
        std::size_t total,
        std::size_t maxLeft,
        std::size_t maxRight
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
            std::size_t codeLength = 1;
            while (total >> ++codeLength)
                ;
            auto code = inputStream->pop(--codeLength);
            if (((code | (1ull << codeLength)) <= total))
                code |= (inputStream->pop_bit() << codeLength);
            left += code;
        }    
        return left;
    }


    //======================================================================================================================
    void split
    (
        input_stream * inputStream,
        std::uint8_t * decodedData,
        std::size_t totalSize,
        std::size_t leftSize,
        symbol_info const (& parentSymbolInfo)[256] 
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
                auto c = inputStream->pop(1);
                decodedData[c == 1] = parentSymbolInfo[1].symbol_;
                decodedData[c == 0] = parentSymbolInfo[0].symbol_; 
            }
            else
            {
                decodedData[0] = parentSymbolInfo[0].symbol_;
            }
            return;
        }

        std::size_t rightSize = (totalSize - leftSize);
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
            auto leftCount = unpack_value(inputStream, totalCount, leftSizeRemaining, rightSizeRemaining);
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
        while (n)
        {
            n -= currentSymbolInfo->count_;
            *c++ = *currentSymbolInfo++;
        }
        split(inputStream + 1, decodedData, leftSize, leftSize >> 1, leftSymbolInfo);
        split(inputStream + 1, decodedData + leftSize, rightSize, rightSize >> 1, rightSymbolInfo);
    }


    //==========================================================================
    void merge
    (
        output_stream * dataStream,
        std::uint8_t const * begin,
        std::size_t totalSize,
        std::size_t leftSize,
        symbol_info (&result)[256],
        std::size_t leadingRunLength
    )
    {
        if (leadingRunLength >= totalSize)
        {
            result[0] = {begin[0], totalSize};
            return;
        }
        if (totalSize <= 2)
        {
            if (totalSize == 2)
            {
                auto c = (unsigned)(begin[0] < begin[1]);
                result[0] = {begin[!c], 1 + (unsigned)(begin[0] == begin[1])};
                result[1] = {begin[c], 1};
                dataStream->push(c, begin[0] != begin[1]);
            }
            else
            {
                result[0] = {begin[0], 1};
            }
            return;
        }

        std::size_t rightSize = (totalSize - leftSize);
        symbol_info left[256];
        symbol_info right[256];
        symbol_info const * current[2] = {left, right};
        symbol_info * resultCurrent = result;
        static auto constexpr leftSide = 0;
        static auto constexpr rightSide = 1;
        auto rightLeadingRunLength = (leadingRunLength > leftSize) ? (leadingRunLength - leftSize) : [](std::uint8_t const * begin, std::uint8_t const * end)
        {
            auto cur = begin;
            auto s = *cur;
            while ((cur < end) && (*cur == s))
                ++cur;
            return std::distance(begin, cur);
        }(begin + leftSize, begin + totalSize);

        merge(dataStream + 1, begin, leftSize, leftSize >> 1, left, leadingRunLength);
        merge(dataStream + 1, begin + leftSize, rightSize, rightSize >> 1, right, rightLeadingRunLength);

        #pragma pack(push, 1)
        using size_union = union 
        {
            std::size_t union_;
            struct
            {
                std::uint32_t left_;
                std::uint32_t right_;
            } size_;
        };
        #pragma pack(pop)
        size_union partitionSize_;
        partitionSize_.size_.left_ = leftSize;
        partitionSize_.size_.right_ = rightSize;

        while (partitionSize_.size_.left_ && partitionSize_.size_.right_)
        {
            size_union count;
            count.size_ = 
            {
                (-(current[leftSide]->symbol_ <= current[rightSide]->symbol_) & (std::uint32_t)current[leftSide]->count_),
                (-(current[rightSide]->symbol_ <= current[leftSide]->symbol_) & (std::uint32_t)current[rightSide]->count_)
            };
            auto totalCount = (count.size_.left_ + count.size_.right_);
            pack_value(dataStream, count.size_.left_, totalCount, partitionSize_.size_.left_, partitionSize_.size_.right_);
            partitionSize_.union_ -= count.union_;
            *resultCurrent++ = {current[(count.size_.left_ == 0)]->symbol_, totalCount};
            current[leftSide] += (count.size_.left_ != 0);
            current[rightSide] += (count.size_.right_ != 0);
        }
        auto n = partitionSize_.size_.left_ + partitionSize_.size_.right_;
        symbol_info const * c = current[(partitionSize_.size_.left_ == 0)];
        while (n)
        {
            n -= c->count_;
            *resultCurrent++ = *c++;
        }
    }

} // namespace


//==========================================================================
auto maniscalco::m99_encode
(
    std::uint8_t const * begin,
    std::uint8_t const * end
) -> std::vector<std::uint8_t>
{
    // determine initial merge boundary (left size is largest power of 2 that is less than the input size).
    std::size_t bytesToEncode = std::distance(begin, end);
    std::size_t leftSize = 1;
    while (leftSize < bytesToEncode)
        leftSize <<= 1;
    symbol_info symbolList[256];

    // do recursive merge and encode
    output_stream headerStream;
    output_stream encodeStream[32];
    auto cur = begin;
    auto s = *cur;
    while ((cur < end) && (*cur == s))
        ++cur;
    auto leadingRunLength = std::distance(begin, cur);
    merge(encodeStream, begin, bytesToEncode, leftSize >> 1, symbolList, leadingRunLength);

    auto sizeBits = 0;
    while ((1ull << sizeBits) <= bytesToEncode)
        ++sizeBits;
    headerStream.push(sizeBits, 5);
    headerStream.push(bytesToEncode, sizeBits);
    auto n = bytesToEncode;
    for (auto & e : symbolList)
    {
        if (n == 0)
            break;
        headerStream.push(e.symbol_, 8);
        pack_value(&headerStream, e.count_, n, n, n);
        n -= e.count_;
    }

    auto estimatedOutputSize = ((headerStream.get_size() + 7) >> 3);
    for (auto & dataStream : encodeStream)
        estimatedOutputSize += ((dataStream.get_size() + 7) >> 3);

    std::vector<std::uint8_t> output;
    output.reserve(estimatedOutputSize + 8192);
    headerStream >> output;
    for (auto const & dataStream : encodeStream)
        dataStream >> output;
    return output;
}


//======================================================================================================================
std::vector<std::uint8_t> maniscalco::m99_decode
(
    std::uint8_t const * inputBegin,
    std::uint8_t const * inputEnd
)
{
    // load the encoded header stream
    input_stream headerStream;
    auto inputCurrent = inputBegin;
    inputCurrent = headerStream.load(inputCurrent, inputEnd);
    // load the encoded sub streams
    input_stream inputStreams[32];
    for (auto & inputStream : inputStreams)
    {
        if (inputCurrent < inputEnd)
            inputCurrent = inputStream.load(inputCurrent, inputEnd);
        else
            break;
    }

    // decode the header stream
    symbol_info symbolInfo[256];
    auto sizeBits = headerStream.pop(5);
    auto bytesToDecode = headerStream.pop(sizeBits);
    auto n = bytesToDecode;
    for (auto & e : symbolInfo)
    {
        if (n == 0)
            break;
        e.symbol_ = headerStream.pop(8);
        e.count_ = unpack_value(&headerStream, n, n, n);
        n -= e.count_;
    }
    std::vector<std::uint8_t> decodedData;
    decodedData.resize(bytesToDecode);
    std::size_t leftSize = 1;
    while (leftSize < bytesToDecode)
        leftSize <<= 1;
    split(inputStreams, decodedData.data(), bytesToDecode, leftSize >> 1, symbolInfo);
    return decodedData;
}

